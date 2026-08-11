/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file wal_recovery.c
 * @brief WAL replay for crash recovery.
 *
 * On startup, reads WAL records sequentially and replays committed
 * mutations into the main database. Uncommitted transactions are
 * skipped (their records are present but never applied). After
 * replay, the WAL is truncated.
 */

#include "wal_recovery.h"
#include "btree_search.h"
#include "btree_modify.h"
#include "record_codec.h"
#include "storage_ops.h"
#include "garry_threading.h"
#include "version_chain.h"
#include "lz4.h"
#include <string.h>
#include <stdlib.h>

#define GARRY_MAX_RECOVERY_COMMITTED 1024

/**
 * @brief Replay WAL records for crash recovery.
 *
 * Performs two passes over the WAL: first collects all committed
 * transaction IDs, then replays update records for those transactions
 * into the main database. Uncommitted transactions are skipped.
 *
 * @param wal WAL log to replay.
 * @param eng Engine handle to apply recovered mutations to.
 *
 * @return GARRY_TRUE on successful recovery, GARRY_FALSE on error.
 */
garry_bool garry_wal_recover(garry_wal_log *wal, garry_engine_handle *eng)
{
    garry_byte rec[GARRY_WAL_RECORD_SIZE];
    garry_i32 bytes_read;
    garry_i32 kind, txid, klen, vlen;
    garry_byte key[GARRY_MAX_RECORD_SIZE];
    garry_byte val[GARRY_MAX_RECORD_SIZE];
    garry_byte lookup_buf[GARRY_MAX_RECORD_SIZE];
    garry_i32 lookup_len;
    garry_i32 cid;
    garry_byte desc_buf[GARRY_DESC_BUF_SIZE];
    garry_i32 desc_len;
    garry_i32 root;
    garry_txn_id *committed;
    garry_i32 commit_count;
    garry_i32 commit_cap;
    garry_i32 i;
    garry_bool found;
    garry_i32 new_is_overflow;

    if (!wal->fd.is_open)
        return GARRY_FALSE;

    commit_cap = GARRY_MAX_RECOVERY_COMMITTED;
    committed = (garry_txn_id *)malloc(sizeof(garry_txn_id) * commit_cap);
    if (!committed)
        return GARRY_FALSE;

    commit_count = 0;
    for (i = 0; i < commit_cap; i++)
        committed[i] = 0;

    /* First pass: collect all committed transaction IDs */
    garry_file_seek(&wal->fd, 0, 0);
    for (;;)
    {
        bytes_read = garry_file_read(&wal->fd, rec, GARRY_WAL_RECORD_SIZE);
        if (bytes_read < GARRY_WAL_RECORD_SIZE)
            break;

        kind = garry_read_int32(rec, WAL_REC_KIND_OFF);
        txid = garry_read_int32(rec, WAL_REC_TXID_OFF);

        if (kind == 1)
        {
            if (commit_count >= commit_cap)
            {
                garry_i32 new_cap = commit_cap * 2;
                garry_txn_id *tmp =
                    (garry_txn_id *)realloc(committed, sizeof(garry_txn_id) * new_cap);
                if (!tmp)
                {
                    free(committed);
                    return GARRY_FALSE;
                }
                committed = tmp;
                for (i = commit_count; i < new_cap; i++)
                    committed[i] = 0;
                commit_cap = new_cap;
            }
            committed[commit_count] = txid;
            commit_count++;

            /* New transactions started after recovery must not reuse an
             * ID that a WAL-recovered commit already used, or MVCC
             * visibility (which orders by txid) breaks. */
            if (txid + 1 > eng->next_txid)
                eng->next_txid = txid + 1;
        }
    }

    /* Second pass: replay update records for committed transactions */
    garry_file_seek(&wal->fd, 0, 0);
    for (;;)
    {
        bytes_read = garry_file_read(&wal->fd, rec, GARRY_WAL_RECORD_SIZE);
        if (bytes_read < GARRY_WAL_RECORD_SIZE)
            break;

        kind = garry_read_int32(rec, WAL_REC_KIND_OFF);
        txid = garry_read_int32(rec, WAL_REC_TXID_OFF);

        if (kind != GARRY_WAL_UPDATE && kind != GARRY_WAL_DELETE)
            continue;

        found = GARRY_FALSE;
        for (i = 0; i < commit_count; i++)
        {
            if (committed[i] == txid)
            {
                found = GARRY_TRUE;
                break;
            }
        }
        if (!found)
            continue;

        klen = garry_read_int32(rec, WAL_REC_KLEN_OFF);
        if (klen < 0 || klen > GARRY_MAX_RECORD_SIZE)
            continue;
        if (WAL_REC_KEY_OFF + klen > GARRY_WAL_RECORD_SIZE)
            continue;
        memcpy(key, rec + WAL_REC_KEY_OFF, (size_t)klen);

        if (kind == GARRY_WAL_DELETE)
        {
            /* Records are replayed in file (chronological) order, so a
             * delete seen here always postdates any earlier update for
             * the same key replayed above — this correctly overrides
             * it, instead of the stale value winning just because
             * deletes were never logged to the WAL at all before this
             * fix (garry_storage_delete only mutated the in-memory
             * chain page, so a deleted-then-crashed key silently
             * reappeared on recovery). */
            garry_rwlock_wrlock(&eng->root_lock);
            lookup_len = 0;
            if (garry_leaf_find_search(eng->pool, eng->btree_root, key, klen, lookup_buf,
                                       &lookup_len))
            {
                garry_i32 chain_id;
                garry_bool has_children;
                garry_decode_descriptor(lookup_buf, lookup_len, &chain_id, &has_children);
                garry_mvcc_delete(eng, txid, chain_id);
            }
            garry_rwlock_wrunlock(&eng->root_lock);
            continue;
        }

        vlen = garry_read_int32(rec, WAL_REC_VLEN_OFF);
        if (vlen < 0 || vlen > GARRY_MAX_RECORD_SIZE)
            continue;

        new_is_overflow = garry_read_int32(rec, WAL_REC_NEW_OVERFLOW_FLAG_OFF);
        if (new_is_overflow)
        {
            garry_i32 overflow_head = garry_read_int32(rec, WAL_REC_NEW_OVERFLOW_HEAD_OFF);
            if (!garry_overflow_read(eng->pool, overflow_head, vlen, (char *)val))
                continue;
        }
        else
        {
            if (WAL_REC_NEW_OFF + vlen > GARRY_WAL_RECORD_SIZE)
                continue;
            memcpy(val, rec + WAL_REC_NEW_OFF, (size_t)vlen);
        }

        garry_rwlock_wrlock(&eng->root_lock);

        lookup_len = 0;
        if (garry_leaf_find_search(eng->pool, eng->btree_root, key, klen, lookup_buf, &lookup_len))
        {
            /* The key is already in the B-tree, meaning some committed
             * version of it is already durably on disk: chain and
             * overflow pages are both flushed synchronously at commit
             * time (see garry_mvcc_commit / garry_overflow_write), not
             * deferred to close. So a key present here needs no replay
             * at all -- re-applying it anyway was not just wasted work,
             * it actively broke recovery for large values: every
             * redundant re-apply allocates a brand-new overflow chain
             * (the old one is never freed), so replaying N large-value
             * commits for an already-current key allocates N times the
             * pages actually needed, and for a database with many large
             * fields (e.g. multi-KB text values) this reliably exhausts
             * the page pool mid-recovery and fails the entire reopen --
             * even though the on-disk data was already fully correct
             * before recovery ever started.
             *
             * But its chain page's ID must still be accounted for: see
             * the pool->next_page comment below. Skipping the entry
             * without also bumping next_page past its chain_id was
             * exactly the bug -- the allocator had no idea this page
             * id was taken until it handed the SAME id out again to a
             * different key later in this same replay pass. */
            garry_i32 chain_id;
            garry_bool has_children;
            garry_decode_descriptor(lookup_buf, lookup_len, &chain_id, &has_children);
            if (chain_id + 1 > eng->pool->next_page)
                eng->pool->next_page = chain_id + 1;
            garry_rwlock_wrunlock(&eng->root_lock);
            continue;
        }

        /* eng->pool->next_page was seeded from eng->header.total_pages,
         * which is only ever written at garry_engine_init() (create)
         * and garry_engine_close() (clean shutdown) -- never during an
         * ongoing live session. After an unclean shutdown that ran for
         * any length of time, that starting value is stale and far too
         * low: hundreds of pages the crashed session actually allocated
         * (chain pages, overflow pages, B-tree leaf/internal pages from
         * splits) are invisible to it. garry_chain_allocate() below
         * calls garry_pool_allocate(), which hands out pool->next_page
         * and increments it -- with no free-list entries yet, it does
         * this completely sequentially from that stale low value,
         * walking straight into page ids that are already legitimately
         * owned by other keys' committed data (silently aliasing two
         * keys onto the same chain page, each overwriting the other's
         * version history). The fix above keeps next_page in sync with
         * every existing chain_id this pass discovers, but that alone
         * only covers ids this same key happens to revisit; the newly
         * allocated id here must also never collide with anything
         * still to be discovered later in the pass, so bump next_page
         * past it immediately, the same as a live session would have
         * (there, pool->next_page is already correctly at high-water
         * mark from every prior allocation in the same run). */
        cid = garry_chain_allocate(eng, key, klen);
        if (cid < 0)
        {
            garry_rwlock_wrunlock(&eng->root_lock);
            free(committed);
            return GARRY_FALSE;
        }
        desc_len = garry_encode_descriptor(cid, GARRY_FALSE, desc_buf);
        root = eng->btree_root;
        garry_btree_insert(eng->pool, &root, key, klen, desc_buf, desc_len);
        eng->btree_root = root;

        /* The WAL record holds the raw, pre-compression value: garry_
         * storage_set() logs the WAL entry from the caller's original
         * bytes and only LZ4-compresses afterward, when writing to the
         * chain page (see storage_ops.c). garry_storage_get() always
         * tries to LZ4-decompress a chain entry's value when
         * eng->compression is enabled, so replaying the WAL's raw bytes
         * directly (as garry_mvcc_recovery_apply does) stores a value
         * that isn't actually compressed -- decompression then fails
         * and every read of it reports not-found, even though recovery
         * itself reports success for the record. Compress here too,
         * to match what a live garry_storage_set() would have stored. */
        if (eng->compression == GARRY_COMPRESS_LZ4)
        {
            size_t compressed_len = 0;
            char *compressed = lz4_compress((const char *)val, (size_t)vlen, &compressed_len);
            garry_bool applied;
            if (!compressed)
            {
                garry_rwlock_wrunlock(&eng->root_lock);
                free(committed);
                return GARRY_FALSE;
            }
            applied = garry_mvcc_recovery_apply(eng, cid, txid, compressed, (garry_i32)compressed_len);
            lz4_free(compressed);
            if (!applied)
            {
                garry_rwlock_wrunlock(&eng->root_lock);
                free(committed);
                return GARRY_FALSE;
            }
        }
        else if (!garry_mvcc_recovery_apply(eng, cid, txid, (const char *)val, vlen))
        {
            garry_rwlock_wrunlock(&eng->root_lock);
            free(committed);
            return GARRY_FALSE;
        }

        garry_rwlock_wrunlock(&eng->root_lock);
    }

    free(committed);
    return GARRY_TRUE;
}
