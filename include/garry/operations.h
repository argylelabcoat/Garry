/**
 * @file operations.h
 * @brief Core CRUD operations for the Garry storage engine.
 *
 * Provides get, set, delete, get-default, and data-inspection functions.
 * All operations require an active transaction handle (see @ref garry_txn_begin).
 *
 * Intent: Expose the minimal set of read/write primitives that every
 * key-value workload needs.
 * Rationale: Keeping the operation surface small makes the engine easier
 * to reason about, test, and optimize.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_OPERATIONS_H
#define GARRY_OPERATIONS_H

#include "garry/export.h"
#include "garry/types.h"

/// Forward declaration — defined in database.h.
typedef struct garry_database garry_database;

/**
 * @brief Retrieve the value associated with a key.
 * @param db     Database handle.
 * @param txn    Active transaction handle.
 * @param key    Pointer to the key bytes.
 * @param klen   Length of @p key in bytes.
 * @param value  Output buffer for the value.
 * @param vlen   [in] capacity of @p value; [out] actual size of the value.
 * @return @ref GARRY_OK on success, @ref GARRY_ERR_NOT_FOUND if the key
 *         does not exist, or another status code on error.
 *
 * If @p value is @c NULL, the function still returns the value size in
 * @p vlen, allowing callers to allocate a correctly sized buffer.
 */
GARRY_API garry_status_t garry_get(garry_database *db, garry_txn txn,
                                    const garry_u8 *key, garry_i32 klen,
                                    garry_u8 *value, garry_i32 *vlen);

/**
 * @brief Insert or update a key-value pair.
 * @param db     Database handle.
 * @param txn    Active transaction handle.
 * @param key    Pointer to the key bytes.
 * @param klen   Length of @p key in bytes.
 * @param value  Pointer to the value bytes.
 * @param vlen   Length of @p value in bytes.
 * @return @ref GARRY_OK on success, or another status code on error.
 *
 * If the key already exists, its value is overwritten atomically.
 */
GARRY_API garry_status_t garry_set(garry_database *db, garry_txn txn,
                                    const garry_u8 *key, garry_i32 klen,
                                    const garry_u8 *value, garry_i32 vlen);

/**
 * @brief Delete a key and its associated value.
 * @param db    Database handle.
 * @param txn   Active transaction handle.
 * @param key   Pointer to the key bytes.
 * @param klen  Length of @p key in bytes.
 * @return @ref GARRY_OK on success, @ref GARRY_ERR_NOT_FOUND if the key
 *         does not exist, or another status code on error.
 *
 * Deleting a nonexistent key is not an error — this function returns
 * @ref GARRY_ERR_NOT_FOUND in that case, consistent with @ref garry_get.
 */
GARRY_API garry_status_t garry_delete(garry_database *db, garry_txn txn,
                                       const garry_u8 *key, garry_i32 klen);

/**
 * @brief Retrieve a value, returning a default if the key is absent.
 * @param db     Database handle.
 * @param txn    Active transaction handle.
 * @param key    Pointer to the key bytes.
 * @param klen   Length of @p key in bytes.
 * @param def    Pointer to the default value bytes.
 * @param dlen   Length of @p def in bytes.
 * @param value  Output buffer for the value (or the default).
 * @param vlen   [in] capacity of @p value; [out] actual size written.
 * @return @ref GARRY_OK on success, or another status code on error.
 *
 * Convenience wrapper: if the key exists, its value is returned; otherwise
 * @p def is copied into @p value and @p vlen is set to @p dlen.
 */
GARRY_API garry_status_t garry_get_default(garry_database *db, garry_txn txn,
                                            const garry_u8 *key, garry_i32 klen,
                                            const garry_u8 *def, garry_i32 dlen,
                                            garry_u8 *value, garry_i32 *vlen);

/**
 * @brief Inspect whether a key has a value, children, or both.
 * @param db    Database handle.
 * @param txn   Active transaction handle.
 * @param key   Pointer to the key bytes.
 * @param klen  Length of @p key in bytes.
 * @return One of @ref GARRY_DATA_NOT_FOUND, @ref GARRY_DATA_HAS_CHILDREN,
 *         @ref GARRY_DATA_HAS_VALUE, or @ref GARRY_DATA_HAS_BOTH.
 *
 * Useful for determining the shape of a key in the B-tree without
 * reading the full value — e.g. to distinguish a directory node from
 * a leaf node.
 */
GARRY_API garry_i32 garry_data(garry_database *db, garry_txn txn,
                                const garry_u8 *key, garry_i32 klen);

#endif /* GARRY_OPERATIONS_H */
