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

garry_i32 garry_decode_cid_from_descriptor(const garry_byte *lookup_result);
garry_bool garry_storage_get(garry_engine_handle *eng, garry_txn_id txn,
                             const garry_byte *key, garry_i32 klen,
                             garry_byte *result, garry_i32 *result_len);
garry_bool garry_storage_set(garry_engine_handle *eng, garry_txn_id txn,
                             const garry_byte *key, garry_i32 klen,
                             const garry_byte *value, garry_i32 vlen);
garry_bool garry_storage_delete(garry_engine_handle *eng, garry_txn_id txn,
                                const garry_byte *key, garry_i32 klen);
garry_bool garry_storage_get_default(garry_engine_handle *eng, garry_txn_id txn,
                                     const garry_byte *key, garry_i32 klen,
                                     const garry_byte *def, garry_i32 dlen,
                                     garry_byte *result, garry_i32 *result_len);
garry_i32 garry_storage_data(garry_engine_handle *eng, garry_txn_id txn,
                             const garry_byte *key, garry_i32 klen);

#endif /* GARRY_STORAGE_OPS_H */
