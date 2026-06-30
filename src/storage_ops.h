/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_ops.h
 * @brief Key-value storage operations (get, set, delete).
 */

#ifndef GARRY_STORAGE_OPS_H
#define GARRY_STORAGE_OPS_H

#include "transaction.h"

/**
 * Decode a chain page ID from a B-tree lookup result descriptor.
 *
 * @param lookup_result Raw descriptor bytes from B-tree search.
 * @return Chain page ID, or -1 if the result is invalid.
 */
garry_i32 garry_decode_cid_from_descriptor(const garry_byte *lookup_result);

/**
 * Read a key's value under MVCC snapshot isolation.
 *
 * @param eng Engine handle.
 * @param txn Transaction ID acting as the snapshot.
 * @param key Key bytes to look up.
 * @param klen Length of the key in bytes.
 * @param result Output buffer for the value bytes.
 * @param result_len Output parameter receiving the actual value length.
 * @return GARRY_TRUE if the key exists with a visible value, GARRY_FALSE otherwise.
 */
garry_bool garry_storage_get(garry_engine_handle *eng, garry_txn_id txn, const garry_byte *key,
                             garry_i32 klen, garry_byte *result, garry_i32 *result_len);

/**
 * Write a key-value pair under MVCC with optional LZ4 compression.
 *
 * If the key does not exist, a new version chain and B-tree entry are
 * created. A WAL record is written before the data mutation.
 *
 * @param eng Engine handle.
 * @param txn Transaction ID performing the write.
 * @param key Key bytes.
 * @param klen Length of the key in bytes.
 * @param value Value bytes to store.
 * @param vlen Length of the value in bytes.
 * @return GARRY_TRUE on success, GARRY_FALSE on failure.
 */
garry_bool garry_storage_set(garry_engine_handle *eng, garry_txn_id txn, const garry_byte *key,
                             garry_i32 klen, const garry_byte *value, garry_i32 vlen);

/**
 * Delete a key by appending a tombstone to its version chain.
 *
 * @param eng Engine handle.
 * @param txn Transaction ID performing the deletion.
 * @param key Key bytes to delete.
 * @param klen Length of the key in bytes.
 * @return GARRY_TRUE on success, GARRY_FALSE on failure.
 */
garry_bool garry_storage_delete(garry_engine_handle *eng, garry_txn_id txn, const garry_byte *key,
                                garry_i32 klen);

/**
 * Read a key's value, returning a default if the key is not found.
 *
 * @param eng Engine handle.
 * @param txn Transaction ID acting as the snapshot.
 * @param key Key bytes to look up.
 * @param klen Length of the key in bytes.
 * @param def Default value bytes to use if the key is missing.
 * @param dlen Length of the default value in bytes.
 * @param result Output buffer for the value bytes.
 * @param result_len Output parameter receiving the actual value length.
 * @return GARRY_TRUE if a value (found or default) was returned.
 */
garry_bool garry_storage_get_default(garry_engine_handle *eng, garry_txn_id txn,
                                     const garry_byte *key, garry_i32 klen, const garry_byte *def,
                                     garry_i32 dlen, garry_byte *result, garry_i32 *result_len);

/**
 * Inspect whether a key has a value, children, or both.
 *
 * @param eng Engine handle.
 * @param txn Transaction ID acting as the snapshot.
 * @param key Key bytes to look up.
 * @param klen Length of the key in bytes.
 * @return One of GARRY_DATA_NOT_FOUND, GARRY_DATA_HAS_CHILDREN,
 *         GARRY_DATA_HAS_VALUE, or GARRY_DATA_HAS_BOTH.
 */
garry_i32 garry_storage_data(garry_engine_handle *eng, garry_txn_id txn, const garry_byte *key,
                             garry_i32 klen);

#endif /* GARRY_STORAGE_OPS_H */
