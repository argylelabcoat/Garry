/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_navigation.h
 * @brief Navigation helpers: first, last, next, prev, exists, count.
 */

#ifndef GARRY_STORAGE_NAVIGATION_H
#define GARRY_STORAGE_NAVIGATION_H

#include "transaction.h"

/**
 * @brief Get the first (smallest) key in the storage engine.
 *
 * Opens an MVCC-visible cursor and returns the first committed key.
 *
 * @param eng   Storage engine handle
 * @param txn   Active transaction ID for visibility
 * @param key   Output buffer for the first key
 * @param klen  Output: length of the first key
 * @return GARRY_TRUE if a key was found, GARRY_FALSE otherwise
 */
garry_bool garry_storage_first(garry_engine_handle *eng, garry_txn_id txn, garry_byte *key,
                               garry_i32 *klen);

/**
 * @brief Get the last (largest) key in the storage engine.
 *
 * Descends to the rightmost leaf and walks backwards to find the
 * largest committed key visible to the given transaction.
 *
 * @param eng   Storage engine handle
 * @param txn   Active transaction ID for visibility
 * @param key   Output buffer for the last key
 * @param klen  Output: length of the last key
 * @return GARRY_TRUE if a key was found, GARRY_FALSE otherwise
 */
garry_bool garry_storage_last(garry_engine_handle *eng, garry_txn_id txn, garry_byte *key,
                              garry_i32 *klen);

/**
 * @brief Get the next key after a given key in sort order.
 *
 * Iterates forward from the beginning until a key strictly greater
 * than @p after is found.
 *
 * @param eng       Storage engine handle
 * @param txn       Active transaction ID for visibility
 * @param after     Key to find the successor of
 * @param after_len Length of @p after in bytes
 * @param key       Output buffer for the next key
 * @param klen      Output: length of the next key
 * @return GARRY_TRUE if a successor key was found, GARRY_FALSE otherwise
 */
garry_bool garry_storage_next_key(garry_engine_handle *eng, garry_txn_id txn,
                                  const garry_byte *after, garry_i32 after_len, garry_byte *key,
                                  garry_i32 *klen);

/**
 * @brief Get the previous key before a given key in sort order.
 *
 * Iterates through all keys and returns the largest key strictly
 * less than @p before.
 *
 * @param eng        Storage engine handle
 * @param txn        Active transaction ID for visibility
 * @param before     Key to find the predecessor of
 * @param before_len Length of @p before in bytes
 * @param key        Output buffer for the previous key
 * @param klen       Output: length of the previous key
 * @return GARRY_TRUE if a predecessor key was found, GARRY_FALSE otherwise
 */
garry_bool garry_storage_prev_key(garry_engine_handle *eng, garry_txn_id txn,
                                  const garry_byte *before, garry_i32 before_len, garry_byte *key,
                                  garry_i32 *klen);

/**
 * @brief Check if a key exists in the storage engine.
 *
 * Searches the B-tree for the key and checks that it has a valid
 * version chain visible to the given transaction.
 *
 * @param eng   Storage engine handle
 * @param txn   Active transaction ID for visibility
 * @param key   Key bytes to look up
 * @param klen  Key length in bytes
 * @return GARRY_TRUE if the key exists, GARRY_FALSE otherwise
 */
garry_bool garry_storage_exists(garry_engine_handle *eng, garry_txn_id txn, const garry_byte *key,
                                garry_i32 klen);

/**
 * @brief Return the total number of keys in the storage engine.
 *
 * Reads the cached key count from the engine header under a read lock.
 * This is an approximate count that includes all committed keys.
 *
 * @param eng  Storage engine handle
 * @param txn  Active transaction ID (unused, for API consistency)
 * @return Number of keys in the engine, or 0 on error
 */
garry_i32 garry_storage_count(garry_engine_handle *eng, garry_txn_id txn);

#endif /* GARRY_STORAGE_NAVIGATION_H */
