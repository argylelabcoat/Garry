/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_cursor.h
 * @brief B-tree cursor for prefix-scoped iteration.
 */

#ifndef GARRY_STORAGE_CURSOR_H
#define GARRY_STORAGE_CURSOR_H

#include "transaction.h"
#include "btree_cursor.h"

typedef struct
{
    garry_btree_cursor_handle btree_cur;
    garry_engine_handle *eng;
    garry_txn_id txn;
} garry_storage_cursor;

/**
 * @brief Open a storage cursor for prefix-scoped iteration.
 *
 * Creates a cursor positioned at the first key matching the given
 * prefix. Pass NULL/0 for prefix and plen to iterate over all keys.
 *
 * @param eng    Storage engine handle
 * @param txn    Active transaction ID for MVCC visibility
 * @param prefix Key prefix to filter by (NULL for all keys)
 * @param plen   Length of the prefix in bytes
 * @return Initialized storage cursor
 */
garry_storage_cursor garry_storage_cursor_open(garry_engine_handle *eng, garry_txn_id txn,
                                               const garry_byte *prefix, garry_i32 plen);

/**
 * @brief Advance the cursor and return the next visible key-value pair.
 *
 * Steps through B-tree entries, resolving MVCC visibility to return
 * only committed values. Skips uncommitted or invisible versions.
 *
 * @param cur   Storage cursor to advance
 * @param key   Output buffer for the key (may be NULL)
 * @param klen  Output: key length in bytes (may be NULL)
 * @param value Output buffer for the value (may be NULL)
 * @param vlen  Output: value length in bytes (may be NULL)
 * @return GARRY_TRUE if a visible key-value pair was found,
 *         GARRY_FALSE if exhausted
 */
garry_bool garry_storage_cursor_next(garry_storage_cursor *cur, garry_byte *key, garry_i32 *klen,
                                     garry_byte *value, garry_i32 *vlen);

/**
 * @brief Peek at the current cursor key without advancing.
 *
 * Returns the key at the cursor's current position without
 * resolving MVCC visibility or moving the cursor forward.
 *
 * @param cur   Storage cursor to peek at
 * @param key   Output buffer for the key
 * @param klen  Output: key length in bytes
 * @return GARRY_TRUE if a key is available, GARRY_FALSE otherwise
 */
garry_bool garry_storage_cursor_peek(garry_storage_cursor *cur, garry_byte *key, garry_i32 *klen);

/**
 * @brief Close the storage cursor.
 *
 * Releases the underlying B-tree cursor resources.
 *
 * @param cur  Storage cursor to close
 */
void garry_storage_cursor_close(garry_storage_cursor *cur);

#endif /* GARRY_STORAGE_CURSOR_H */
