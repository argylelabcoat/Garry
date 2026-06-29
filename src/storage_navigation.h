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

garry_bool garry_storage_first(garry_engine_handle *eng, garry_txn_id txn,
                               garry_byte *key, garry_i32 *klen);
garry_bool garry_storage_last(garry_engine_handle *eng, garry_txn_id txn,
                              garry_byte *key, garry_i32 *klen);
garry_bool garry_storage_next_key(garry_engine_handle *eng, garry_txn_id txn,
                                  const garry_byte *after, garry_i32 after_len,
                                  garry_byte *key, garry_i32 *klen);
garry_bool garry_storage_prev_key(garry_engine_handle *eng, garry_txn_id txn,
                                  const garry_byte *before, garry_i32 before_len,
                                  garry_byte *key, garry_i32 *klen);
garry_bool garry_storage_exists(garry_engine_handle *eng, garry_txn_id txn,
                                const garry_byte *key, garry_i32 klen);
garry_i32 garry_storage_count(garry_engine_handle *eng, garry_txn_id txn);

#endif /* GARRY_STORAGE_NAVIGATION_H */
