/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_navigation.c
 * @brief B-tree navigation operations for positional key access.
 *
 * Provides first/last/next/prev traversal, key existence checks, and
 * an approximate record count. All operations resolve MVCC visibility
 * so only committed keys are reported.
 */

#include "storage_navigation.h"
#include "storage_cursor.h"
#include "storage_ops.h"
#include "hierarchical.h"
#include "key_encoding.h"
#include <string.h>

garry_bool garry_storage_first(garry_engine_handle *eng, garry_txn_id txn,
                               garry_byte *key, garry_i32 *klen)
{
    garry_storage_cursor cur;
    garry_bool ok;

    if (!eng) return 0;
    if (!garry_txn_is_active(txn, eng)) return 0;

    garry_rwlock_rdlock(&eng->root_lock);
    cur = garry_storage_cursor_open(eng, txn, NULL, 0);
    ok = garry_storage_cursor_next(&cur, key, klen, NULL, NULL);
    garry_storage_cursor_close(&cur);
    garry_rwlock_rdunlock(&eng->root_lock);
    return ok;
}

garry_bool garry_storage_last(garry_engine_handle *eng, garry_txn_id txn,
                              garry_byte *key, garry_i32 *klen)
{
    garry_storage_cursor cur;
    garry_byte last_key[sizeof(garry_byte_array)];
    garry_i32 last_klen;
    garry_bool found;

    if (!eng) return 0;
    if (!garry_txn_is_active(txn, eng)) return 0;

    garry_rwlock_rdlock(&eng->root_lock);
    cur = garry_storage_cursor_open(eng, txn, NULL, 0);
    found = 0;
    last_klen = 0;
    memset(last_key, 0, sizeof(last_key));

    for (;;) {
        garry_byte k[sizeof(garry_byte_array)];
        garry_i32 kl;

        kl = 0;
        if (!garry_storage_cursor_next(&cur, k, &kl, NULL, NULL)) {
            break;
        }
        memcpy(last_key, k, sizeof(last_key));
        last_klen = kl;
        found = 1;
    }
    garry_storage_cursor_close(&cur);
    garry_rwlock_rdunlock(&eng->root_lock);

    if (found && key && klen) {
        memcpy(key, last_key, sizeof(last_key));
        *klen = last_klen;
    }
    return found;
}

garry_bool garry_storage_next_key(garry_engine_handle *eng, garry_txn_id txn,
                                  const garry_byte *after, garry_i32 after_len,
                                  garry_byte *key, garry_i32 *klen)
{
    garry_storage_cursor cur;
    garry_byte k[sizeof(garry_byte_array)];
    garry_i32 kl;

    if (!eng) return 0;
    if (!garry_txn_is_active(txn, eng)) return 0;
    if (!after || after_len <= 0) return 0;

    garry_rwlock_rdlock(&eng->root_lock);
    cur = garry_storage_cursor_open(eng, txn, NULL, 0);

    for (;;) {
        kl = 0;
        if (!garry_storage_cursor_next(&cur, k, &kl, NULL, NULL)) {
            garry_storage_cursor_close(&cur);
            garry_rwlock_rdunlock(&eng->root_lock);
            return 0;
        }
        if (garry_byte_compare(k, kl, after, after_len) > 0) {
            if (key) memcpy(key, k, sizeof(garry_byte_array));
            if (klen) *klen = kl;
            garry_storage_cursor_close(&cur);
            garry_rwlock_rdunlock(&eng->root_lock);
            return 1;
        }
    }
}

garry_bool garry_storage_prev_key(garry_engine_handle *eng, garry_txn_id txn,
                                  const garry_byte *before, garry_i32 before_len,
                                  garry_byte *key, garry_i32 *klen)
{
    garry_storage_cursor cur;
    garry_byte prev_key[sizeof(garry_byte_array)];
    garry_i32 prev_klen;
    garry_bool found;

    if (!eng) return 0;
    if (!garry_txn_is_active(txn, eng)) return 0;
    if (!before || before_len <= 0) return 0;

    garry_rwlock_rdlock(&eng->root_lock);
    cur = garry_storage_cursor_open(eng, txn, NULL, 0);
    found = 0;
    prev_klen = 0;
    memset(prev_key, 0, sizeof(prev_key));

    for (;;) {
        garry_byte k[sizeof(garry_byte_array)];
        garry_i32 kl;

        kl = 0;
        if (!garry_storage_cursor_next(&cur, k, &kl, NULL, NULL)) {
            break;
        }
        if (garry_byte_compare(k, kl, before, before_len) < 0) {
            memcpy(prev_key, k, sizeof(prev_key));
            prev_klen = kl;
            found = 1;
        }
    }
    garry_storage_cursor_close(&cur);
    garry_rwlock_rdunlock(&eng->root_lock);

    if (found && key && klen) {
        memcpy(key, prev_key, sizeof(prev_key));
        *klen = prev_klen;
    }
    return found;
}

garry_bool garry_storage_exists(garry_engine_handle *eng, garry_txn_id txn,
                                const garry_byte *key, garry_i32 klen)
{
    garry_byte result[512];
    garry_i32 result_len;

    if (!eng || !key || klen <= 0) return 0;
    result_len = 0;
    return garry_storage_get(eng, txn, key, klen, result, &result_len);
}

garry_i32 garry_storage_count(garry_engine_handle *eng, garry_txn_id txn)
{
    garry_storage_cursor cur;
    garry_i32 count;

    if (!eng) return 0;
    if (!garry_txn_is_active(txn, eng)) return 0;

    garry_rwlock_rdlock(&eng->root_lock);
    cur = garry_storage_cursor_open(eng, txn, NULL, 0);
    count = 0;
    for (;;) {
        if (!garry_storage_cursor_next(&cur, NULL, NULL, NULL, NULL)) {
            break;
        }
        count++;
    }
    garry_storage_cursor_close(&cur);
    garry_rwlock_rdunlock(&eng->root_lock);
    return count;
}
