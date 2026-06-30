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
#include "btree_search.h"
#include "hierarchical.h"
#include "key_encoding.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief Retrieve the first visible key in the B-tree.
 *
 * Opens a cursor and returns the first key-value pair that is visible
 * under the given transaction's MVCC snapshot.
 *
 * @param eng   Engine handle.
 * @param txn   Transaction ID for visibility.
 * @param key   Buffer to receive the first key.
 * @param klen  Output parameter for key length.
 * @return 1 if a visible key was found, 0 otherwise.
 */
garry_bool garry_storage_first(garry_engine_handle *eng, garry_txn_id txn, garry_byte *key,
                               garry_i32 *klen)
{
    garry_storage_cursor cur;
    garry_bool ok;

    if (!eng)
        return 0;
    if (!garry_txn_is_active(txn, eng))
        return 0;

    /* cursor_open and cursor_next manage their own root_lock internally.
     * Do NOT wrap in an outer rdlock — that would nested-lock the rwlock,
     * which deadlocks on non-recursive implementations (macOS, most Linux). */
    cur = garry_storage_cursor_open(eng, txn, NULL, 0);
    ok = garry_storage_cursor_next(&cur, key, klen, NULL, NULL);
    garry_storage_cursor_close(&cur);
    return ok;
}

/**
 * @brief Retrieve the last visible key in the B-tree.
 *
 * Traverses to the rightmost leaf and walks entries in reverse to
 * find the last key visible under the given transaction's MVCC snapshot.
 *
 * @param eng   Engine handle.
 * @param txn   Transaction ID for visibility.
 * @param key   Buffer to receive the last key.
 * @param klen  Output parameter for key length.
 * @return 1 if a visible key was found, 0 otherwise.
 */
garry_bool garry_storage_last(garry_engine_handle *eng, garry_txn_id txn, garry_byte *key,
                               garry_i32 *klen)
{
    garry_btree_node node;
    garry_i32 page;

    if (!eng)
        return 0;
    if (!garry_txn_is_active(txn, eng))
        return 0;

    garry_rwlock_rdlock(&eng->root_lock);

    page = eng->btree_root;
    for (;;)
    {
        garry_load_node(eng->pool, page, &node);
        if (node.kind == GARRY_NODE_LEAF)
            break;
        if (node.entry_count == 0)
        {
            garry_rwlock_rdunlock(&eng->root_lock);
            return 0;
        }
        page = node.children[node.entry_count];
    }

    if (node.entry_count > 0)
    {
        garry_byte lookup[GARRY_LOOKUP_BUF_SIZE];
        garry_i32 lookup_len;
        garry_i32 cid;
        char *val;
        garry_i32 mv_len;
        garry_i32 idx;

        for (idx = node.entry_count - 1; idx >= 0; idx--)
        {
            lookup_len = node.value_lens[idx];
            if (lookup_len > 0 && lookup_len <= GARRY_LOOKUP_BUF_SIZE)
            {
                memcpy(lookup, node.values[idx], (size_t)lookup_len);
                cid = garry_decode_cid_from_descriptor(lookup);
                if (cid >= 0)
                {
                    val = garry_mvcc_get(eng, txn, cid, &mv_len);
                    if (val)
                    {
                        if (key)
                            memcpy(key, node.keys[idx], sizeof(garry_byte_array));
                        if (klen)
                            *klen = node.key_lens[idx];
                        free(val);
                        garry_rwlock_rdunlock(&eng->root_lock);
                        return 1;
                    }
                }
            }
        }
    }

    garry_rwlock_rdunlock(&eng->root_lock);
    return 0;
}

/**
 * @brief Find the next visible key strictly greater than a given key.
 *
 * Iterates from the start until a key lexicographically greater than
 * @p after is found that is visible under the given transaction.
 *
 * @param eng       Engine handle.
 * @param txn       Transaction ID for visibility.
 * @param after     Key to search after.
 * @param after_len Length of @p after.
 * @param key       Buffer to receive the next key.
 * @param klen      Output parameter for key length.
 * @return 1 if a visible next key was found, 0 otherwise.
 */
garry_bool garry_storage_next_key(garry_engine_handle *eng, garry_txn_id txn,
                                   const garry_byte *after, garry_i32 after_len, garry_byte *key,
                                   garry_i32 *klen)
{
    garry_storage_cursor cur;
    garry_byte k[sizeof(garry_byte_array)];
    garry_i32 kl;

    if (!eng)
        return 0;
    if (!garry_txn_is_active(txn, eng))
        return 0;
    if (!after || after_len <= 0)
        return 0;

    /* cursor_open and cursor_next manage their own root_lock internally. */
    cur = garry_storage_cursor_open(eng, txn, NULL, 0);

    for (;;)
    {
        kl = 0;
        if (!garry_storage_cursor_next(&cur, k, &kl, NULL, NULL))
        {
            garry_storage_cursor_close(&cur);
            return 0;
        }
        if (garry_byte_compare(k, kl, after, after_len) > 0)
        {
            if (key)
                memcpy(key, k, sizeof(garry_byte_array));
            if (klen)
                *klen = kl;
            garry_storage_cursor_close(&cur);
            return 1;
        }
    }
}

/**
 * @brief Find the previous visible key strictly less than a given key.
 *
 * Scans all visible keys and returns the one closest to but lexicographically
 * less than @p before.
 *
 * @param eng        Engine handle.
 * @param txn        Transaction ID for visibility.
 * @param before     Key to search before.
 * @param before_len Length of @p before.
 * @param key        Buffer to receive the previous key.
 * @param klen       Output parameter for key length.
 * @return 1 if a visible previous key was found, 0 otherwise.
 */
garry_bool garry_storage_prev_key(garry_engine_handle *eng, garry_txn_id txn,
                                   const garry_byte *before, garry_i32 before_len, garry_byte *key,
                                   garry_i32 *klen)
{
    garry_storage_cursor cur;
    garry_byte prev_key[sizeof(garry_byte_array)];
    garry_i32 prev_klen;
    garry_bool found;

    if (!eng)
        return 0;
    if (!garry_txn_is_active(txn, eng))
        return 0;
    if (!before || before_len <= 0)
        return 0;

    /* cursor_open and cursor_next manage their own root_lock internally. */
    cur = garry_storage_cursor_open(eng, txn, NULL, 0);
    found = 0;
    prev_klen = 0;
    memset(prev_key, 0, sizeof(prev_key));

    for (;;)
    {
        garry_byte k[sizeof(garry_byte_array)];
        garry_i32 kl;

        kl = 0;
        if (!garry_storage_cursor_next(&cur, k, &kl, NULL, NULL))
        {
            break;
        }
        if (garry_byte_compare(k, kl, before, before_len) < 0)
        {
            memcpy(prev_key, k, sizeof(prev_key));
            prev_klen = kl;
            found = 1;
        }
    }
    garry_storage_cursor_close(&cur);

    if (found && key && klen)
    {
        memcpy(key, prev_key, sizeof(prev_key));
        *klen = prev_klen;
    }
    return found;
}

/**
 * @brief Check whether a key exists with a visible version.
 *
 * Performs a B-tree leaf lookup and decodes the version chain descriptor
 * to determine if the key is present under MVCC visibility.
 *
 * @param eng   Engine handle.
 * @param txn   Transaction ID for visibility.
 * @param key   Key to look up.
 * @param klen  Key length.
 * @return 1 if the key exists and is visible, 0 otherwise.
 */
garry_bool garry_storage_exists(garry_engine_handle *eng, garry_txn_id txn, const garry_byte *key,
                                 garry_i32 klen)
{
    garry_byte lookup[GARRY_LOOKUP_BUF_SIZE];
    garry_i32 lookup_len;
    garry_i32 cid;

    if (!eng || !key || klen <= 0)
        return 0;
    if (!garry_txn_is_active(txn, eng))
        return 0;

    garry_rwlock_rdlock(&eng->root_lock);

    lookup_len = 0;
    memset(lookup, 0, sizeof(lookup));
    if (!garry_leaf_find_search(eng->pool, eng->btree_root, key, klen, lookup, &lookup_len))
    {
        garry_rwlock_rdunlock(&eng->root_lock);
        return 0;
    }

    cid = garry_decode_cid_from_descriptor(lookup);
    garry_rwlock_rdunlock(&eng->root_lock);

    return (cid >= 0) ? 1 : 0;
}

/**
 * @brief Return the approximate count of visible keys.
 *
 * Reads the engine's key count under a read lock. This is an
 * approximate count that does not account for MVCC tombstones
 * or uncommitted entries.
 *
 * @param eng  Engine handle.
 * @param txn  Transaction ID (used for active-transaction check).
 * @return Approximate key count, or 0 if the engine is inactive.
 */
garry_i32 garry_storage_count(garry_engine_handle *eng, garry_txn_id txn)
{
    garry_i32 count;

    if (!eng)
        return 0;
    if (!garry_txn_is_active(txn, eng))
        return 0;

    garry_rwlock_rdlock(&eng->root_lock);
    count = eng->key_count;
    garry_rwlock_rdunlock(&eng->root_lock);
    return count;
}
