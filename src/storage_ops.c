/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_ops.c
 * @brief Core get/set/delete operations with optional compression.
 *
 * Each operation follows this pipeline:
 *   1. B-tree lookup to find the version chain page ID (descriptor).
 *   2. MVCC visibility check to get the visible version.
 *   3. Optional LZ4 decompress on read, compress on write.
 *   4. Delete appends a tombstone to the version chain.
 *
 * Compression is transparent: callers never see compressed payloads.
 * Lock acquisition happens at the key level before the root write lock.
 */

#include "storage_ops.h"
#include "btree_search.h"
#include "btree_modify.h"
#include "record_codec.h"
#include "buffer_pool.h"
#include "lock_table.h"
#include "wal_log.h"
#include "wal_record.h"
#include "lz4.h"
#include <string.h>
#include <stdlib.h>

garry_i32 garry_decode_cid_from_descriptor(const garry_byte *lookup_result)
{
    garry_i32 chain_id;
    garry_bool has_children;

    if (!lookup_result) return -1;

    if ((unsigned char)lookup_result[0] == (GARRY_CBOR_MAP_BASE + 2)) {
        garry_decode_descriptor(lookup_result, GARRY_LOOKUP_BUF_SIZE, &chain_id, &has_children);
        return chain_id;
    }

    return (garry_i32)lookup_result[0];
}

/**
 * Read a key's value under MVCC snapshot isolation.
 *
 * Pipeline: B-tree find → decode chain ID → MVCC get → decompress.
 * Acquires a read lock on the root to prevent concurrent tree mutations.
 * Returns true if the key exists and has a visible, non-tombstone value.
 */
garry_bool garry_storage_get(garry_engine_handle *eng, garry_txn_id txn,
                             const garry_byte *key, garry_i32 klen,
                             garry_byte *result, garry_i32 *result_len)
{
    garry_byte lookup[GARRY_LOOKUP_BUF_SIZE];
    garry_i32 lookup_len;
    garry_i32 cid;
    char *val;
    garry_i32 vlen;

    if (!eng || !key || klen <= 0) return 0;
    if (klen > GARRY_MAX_KEY_SIZE) return 0;
    if (!garry_txn_is_active(txn, eng)) return 0;

    garry_rwlock_rdlock(&eng->root_lock);

    lookup_len = 0;
    memset(lookup, 0, sizeof(lookup));
    if (!garry_leaf_find_search(eng->pool, eng->btree_root,
                                key, klen, lookup, &lookup_len)) {
        garry_rwlock_rdunlock(&eng->root_lock);
        return 0;
    }

    cid = garry_decode_cid_from_descriptor(lookup);
    if (cid < 0) {
        garry_rwlock_rdunlock(&eng->root_lock);
        return 0;
    }

    val = garry_mvcc_get(eng, txn, cid, &vlen);

    garry_rwlock_rdunlock(&eng->root_lock);

    if (!val) return 0;

    if (eng->compression == GARRY_COMPRESS_LZ4) {
        size_t decompressed_len = 0;
        char *decompressed = lz4_decompress(val, (size_t)vlen,
                                           (size_t)GARRY_MAX_RECORD_SIZE,
                                           &decompressed_len);
        free(val);
        if (!decompressed) return 0;
        if (decompressed_len > (size_t)GARRY_MAX_RECORD_SIZE) {
            lz4_free(decompressed);
            return 0;
        }
        memcpy(result, decompressed, decompressed_len);
        lz4_free(decompressed);
        *result_len = (garry_i32)decompressed_len;
    }
    else {
        memcpy(result, val, (size_t)vlen);
        free(val);
        *result_len = vlen;
    }
    return 1;
}

/**
 * Write a key-value pair under MVCC.
 *
 * Pipeline: acquire exclusive key lock → B-tree find-or-create →
 * compress value → MVCC set → WAL append. If the key doesn't exist,
 * allocates a new version chain and inserts a B-tree descriptor.
 * The WAL record is written after the data commit for crash recovery.
 */
garry_bool garry_storage_set(garry_engine_handle *eng, garry_txn_id txn,
                             const garry_byte *key, garry_i32 klen,
                             const garry_byte *value, garry_i32 vlen)
{
    garry_byte lookup[GARRY_LOOKUP_BUF_SIZE];
    garry_i32 lookup_len;
    garry_i32 cid;
    garry_byte desc[GARRY_DESC_BUF_SIZE];
    garry_i32 desc_len;
    garry_bool ok;
    garry_wal_record *rec;

    if (!eng || !key || klen <= 0 || !value || vlen <= 0) return 0;
    if (klen > GARRY_MAX_KEY_SIZE) return 0;
    if (!garry_txn_is_active(txn, eng)) return 0;

    garry_lock_acquire(&eng->lock_mgr, txn, key, klen,
                       GARRY_LOCK_EXCLUSIVE, &ok);
    if (!ok) return 0;

    rec = garry_make_update_record(txn, key, klen, value, vlen);
    if (rec) {
        garry_wal_log_append(&eng->wal, rec);
        garry_wal_record_free(rec);
    }

    garry_rwlock_wrlock(&eng->root_lock);

    lookup_len = 0;
    memset(lookup, 0, sizeof(lookup));
    if (garry_leaf_find_search(eng->pool, eng->btree_root,
                               key, klen, lookup, &lookup_len)) {
        cid = garry_decode_cid_from_descriptor(lookup);
    } else {
        cid = garry_chain_allocate(eng, key, klen);
        if (cid < 0) {
            garry_rwlock_wrunlock(&eng->root_lock);
            return 0;
        }
        desc_len = garry_encode_descriptor(cid, 0, desc);
        if (!garry_btree_insert(eng->pool, &eng->btree_root,
                                key, klen, desc, desc_len)) {
            garry_rwlock_wrunlock(&eng->root_lock);
            return 0;
        }
    }

    if (eng->compression == GARRY_COMPRESS_LZ4) {
        size_t compressed_len = 0;
        char *compressed = lz4_compress((const char*)value, (size_t)vlen,
                                        &compressed_len);
        if (!compressed) {
            garry_rwlock_wrunlock(&eng->root_lock);
            return 0;
        }
        if (!garry_mvcc_set(eng, txn, cid, compressed, (garry_i32)compressed_len)) {
            lz4_free(compressed);
            garry_rwlock_wrunlock(&eng->root_lock);
            return 0;
        }
        lz4_free(compressed);
    }
    else {
        if (!garry_mvcc_set(eng, txn, cid, (const char*)value, vlen)) {
            garry_rwlock_wrunlock(&eng->root_lock);
            return 0;
        }
    }

    garry_rwlock_wrunlock(&eng->root_lock);

    return 1;
}

/**
 * Delete a key by appending a tombstone to its version chain.
 *
 * Does not remove the B-tree entry — the tombstone marks the key as
 * deleted for MVCC readers. A subsequent garbage collection pass can
 * reclaim the chain and B-tree slot once no active transactions reference it.
 */
garry_bool garry_storage_delete(garry_engine_handle *eng, garry_txn_id txn,
                                const garry_byte *key, garry_i32 klen)
{
    garry_byte lookup[GARRY_LOOKUP_BUF_SIZE];
    garry_i32 lookup_len;
    garry_i32 cid;
    garry_bool ok;

    if (!eng || !key || klen <= 0) return 0;
    if (!garry_txn_is_active(txn, eng)) return 0;

    garry_lock_acquire(&eng->lock_mgr, txn, key, klen,
                       GARRY_LOCK_EXCLUSIVE, &ok);
    if (!ok) return 0;

    garry_rwlock_wrlock(&eng->root_lock);

    lookup_len = 0;
    memset(lookup, 0, sizeof(lookup));
    if (!garry_leaf_find_search(eng->pool, eng->btree_root,
                                key, klen, lookup, &lookup_len)) {
        garry_rwlock_wrunlock(&eng->root_lock);
        return 0;
    }

    cid = garry_decode_cid_from_descriptor(lookup);
    if (cid < 0) {
        garry_rwlock_wrunlock(&eng->root_lock);
        return 0;
    }

    ok = garry_mvcc_delete(eng, txn, cid);
    garry_rwlock_wrunlock(&eng->root_lock);
    return ok;
}

garry_bool garry_storage_get_default(garry_engine_handle *eng, garry_txn_id txn,
                                     const garry_byte *key, garry_i32 klen,
                                     const garry_byte *def, garry_i32 dlen,
                                     garry_byte *result, garry_i32 *result_len)
{
    if (garry_storage_get(eng, txn, key, klen, result, result_len)) {
        return 1;
    }
    if (!garry_txn_is_active(txn, eng)) return 0;
    if (dlen > GARRY_MAX_RECORD_SIZE) return 0;
    memcpy(result, def, (size_t)dlen);
    *result_len = dlen;
    return 1;
}

garry_i32 garry_storage_data(garry_engine_handle *eng, garry_txn_id txn,
                             const garry_byte *key, garry_i32 klen)
{
    garry_byte lookup[GARRY_LOOKUP_BUF_SIZE];
    garry_i32 lookup_len;
    garry_i32 cid;
    garry_bool has_children;
    char *val;
    garry_i32 vlen;

    if (!eng || !key || klen <= 0) return 0;

    garry_rwlock_rdlock(&eng->root_lock);

    lookup_len = 0;
    memset(lookup, 0, sizeof(lookup));
    if (!garry_leaf_find_search(eng->pool, eng->btree_root,
                                key, klen, lookup, &lookup_len)) {
        garry_rwlock_rdunlock(&eng->root_lock);
        return 0;
    }

    has_children = 0;
    cid = garry_decode_cid_from_descriptor(lookup);
    if ((unsigned char)lookup[0] == (GARRY_CBOR_MAP_BASE + 2)) {
        garry_decode_descriptor(lookup, GARRY_LOOKUP_BUF_SIZE, &cid, &has_children);
    }

    val = garry_mvcc_get(eng, txn, cid, &vlen);

    garry_rwlock_rdunlock(&eng->root_lock);

    if (val && has_children) return 3;
    if (val) return 2;
    if (has_children) return 1;
    return 0;
}
