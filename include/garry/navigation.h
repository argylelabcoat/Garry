/**
 * @file navigation.h
 * @brief Key-space navigation functions for the Garry storage engine.
 *
 * Provides first/last/next/prev traversal, existence checks, and key
 * counting without requiring a persistent cursor. These functions are
 * useful for one-off lookups where cursor overhead is unnecessary.
 *
 * Intent: Offer lightweight key-space navigation for callers that need
 * occasional adjacency queries without maintaining cursor state.
 * Rationale: Not every use case needs cursor state; these functions
 * provide simpler, stateless alternatives for simple traversal.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_NAVIGATION_H
#define GARRY_NAVIGATION_H

#include "garry/export.h"
#include "garry/types.h"

/// Forward declaration — defined in database.h.
#ifndef GARRY_DATABASE_FWD_DEFINED
#define GARRY_DATABASE_FWD_DEFINED
typedef struct garry_database garry_database;
#endif

/**
 * @brief Retrieve the lexicographically first key in the database.
 * @param db    Database handle.
 * @param txn   Active transaction handle.
 * @param key   Output buffer for the key.
 * @param klen  [in] capacity of @p key; [out] actual key size.
 * @return @ref GARRY_TRUE if a key was found, @ref GARRY_FALSE if the
 *         database is empty.
 */
GARRY_API garry_bool garry_first(garry_database *db, garry_txn txn,
                                   garry_u8 *key, garry_i32 *klen);

/**
 * @brief Retrieve the lexicographically last key in the database.
 * @param db    Database handle.
 * @param txn   Active transaction handle.
 * @param key   Output buffer for the key.
 * @param klen  [in] capacity of @p key; [out] actual key size.
 * @return @ref GARRY_TRUE if a key was found, @ref GARRY_FALSE if the
 *         database is empty.
 */
GARRY_API garry_bool garry_last(garry_database *db, garry_txn txn,
                                  garry_u8 *key, garry_i32 *klen);

/**
 * @brief Find the next key after a given key.
 * @param db         Database handle.
 * @param txn        Active transaction handle.
 * @param after      Key to start searching after (exclusive).
 * @param after_len  Length of @p after in bytes.
 * @param key        Output buffer for the next key.
 * @param klen       [in] capacity of @p key; [out] actual key size.
 * @return @ref GARRY_TRUE if a next key exists, @ref GARRY_FALSE if
 *         @p after is the last key.
 */
GARRY_API garry_bool garry_next_key(garry_database *db, garry_txn txn,
                                     const garry_u8 *after, garry_i32 after_len,
                                     garry_u8 *key, garry_i32 *klen);

/**
 * @brief Find the previous key before a given key.
 * @param db         Database handle.
 * @param txn        Active transaction handle.
 * @param before     Key to start searching before (exclusive).
 * @param before_len Length of @p before in bytes.
 * @param key        Output buffer for the previous key.
 * @param klen       [in] capacity of @p key; [out] actual key size.
 * @return @ref GARRY_TRUE if a previous key exists, @ref GARRY_FALSE if
 *         @p before is the first key.
 */
GARRY_API garry_bool garry_prev_key(garry_database *db, garry_txn txn,
                                     const garry_u8 *before, garry_i32 before_len,
                                     garry_u8 *key, garry_i32 *klen);

/**
 * @brief Check whether a key exists in the database.
 * @param db    Database handle.
 * @param txn   Active transaction handle.
 * @param key   Pointer to the key bytes.
 * @param klen  Length of @p key in bytes.
 * @return @ref GARRY_TRUE if the key exists, @ref GARRY_FALSE otherwise.
 *
 * This is a lightweight check — it does not read the value, only the
 * key's presence in the B-tree.
 */
GARRY_API garry_bool garry_exists(garry_database *db, garry_txn txn,
                                    const garry_u8 *key, garry_i32 klen);

/**
 * @brief Count the total number of keys in the database.
 * @param db   Database handle.
 * @param txn  Active transaction handle.
 * @return Number of keys, or 0 if the database is empty.
 *
 * Iterates the entire key space — O(n) in the number of keys.
 */
GARRY_API garry_i32 garry_count(garry_database *db, garry_txn txn);

#endif /* GARRY_NAVIGATION_H */
