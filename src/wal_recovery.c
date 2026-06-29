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
#include <string.h>

#ifndef GARRY_TRUE
#define GARRY_TRUE 1
#endif
#ifndef GARRY_FALSE
#define GARRY_FALSE 0
#endif

#define WAL_REC_SIZE 1552
#define WAL_REC_KIND_OFF  0
#define WAL_REC_TXID_OFF  4
#define WAL_REC_KLEN_OFF  8
#define WAL_REC_VLEN_OFF  12
#define WAL_REC_KEY_OFF   16
#define WAL_REC_OLD_OFF   528
#define WAL_REC_NEW_OFF   1040

#define MAX_COMMITTED 256

garry_bool garry_wal_recover(garry_wal_log *wal, garry_engine_handle *eng)
{
    garry_byte rec[WAL_REC_SIZE];
    garry_i32 bytes_read;
    garry_i32 kind, txid, klen, vlen;
    garry_byte key[512];
    garry_byte val[512];
    garry_byte lookup_buf[512];
    garry_i32 lookup_len;
    garry_i32 cid;
    garry_byte desc_buf[64];
    garry_i32 desc_len;
    garry_i32 root;
    garry_txn_id committed[MAX_COMMITTED];
    garry_i32 commit_count;
    garry_i32 i;
    garry_bool found;

    if (!wal->fd.is_open) return GARRY_FALSE;

    commit_count = 0;
    for (i = 0; i < MAX_COMMITTED; i++) committed[i] = 0;

    garry_file_seek(&wal->fd, 0, 0);
    for (;;) {
        bytes_read = garry_file_read(&wal->fd, rec, WAL_REC_SIZE);
        if (bytes_read < WAL_REC_SIZE) break;

        kind = *((garry_i32*)(rec + WAL_REC_KIND_OFF));
        txid = *((garry_i32*)(rec + WAL_REC_TXID_OFF));

        if (kind == 1 && commit_count < MAX_COMMITTED) {
            committed[commit_count] = txid;
            commit_count++;
        }
    }

    garry_file_seek(&wal->fd, 0, 0);
    for (;;) {
        bytes_read = garry_file_read(&wal->fd, rec, WAL_REC_SIZE);
        if (bytes_read < WAL_REC_SIZE) break;

        kind = *((garry_i32*)(rec + WAL_REC_KIND_OFF));
        txid = *((garry_i32*)(rec + WAL_REC_TXID_OFF));

        if (kind != 0) continue;

        found = GARRY_FALSE;
        for (i = 0; i < commit_count; i++) {
            if (committed[i] == txid) { found = GARRY_TRUE; break; }
        }
        if (!found) continue;

        klen = *((garry_i32*)(rec + WAL_REC_KLEN_OFF));
        vlen = *((garry_i32*)(rec + WAL_REC_VLEN_OFF));
        memcpy(key, rec + WAL_REC_KEY_OFF, (size_t)klen);
        memcpy(val, rec + WAL_REC_NEW_OFF, (size_t)vlen);

        lookup_len = 0;
        if (garry_leaf_find_search(eng->pool, eng->btree_root, key, klen,
                                   lookup_buf, &lookup_len)) {
            garry_i32 chain_id;
            garry_bool has_children;
            garry_decode_descriptor(lookup_buf, lookup_len, &chain_id, &has_children);
            cid = chain_id;
        } else {
            cid = garry_chain_allocate(eng, key, klen);
            if (cid < 0) return GARRY_FALSE;
            desc_len = garry_encode_descriptor(cid, GARRY_FALSE, desc_buf);
            root = eng->btree_root;
            garry_btree_insert(eng->pool, &root, key, klen, desc_buf, desc_len);
            eng->btree_root = root;
        }

        if (!garry_mvcc_recovery_apply(eng, cid, txid, (const char*)val, vlen))
            return GARRY_FALSE;
    }

    return GARRY_TRUE;
}
