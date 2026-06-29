/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file version_chain.h
 * @brief Per-key version chain for MVCC storage.
 */

#ifndef GARRY_VERSION_CHAIN_H
#define GARRY_VERSION_CHAIN_H

#include "garry/config.h"
#include "storage_types.h"
#include "storage_core.h"
#include "buffer_pool.h"

typedef garry_txn_id* garry_txn_id_ptr;

void garry_chain_page_init(garry_page_buffer buf, garry_u32 page_size);
garry_bool garry_chain_page_append(garry_page_buffer buf, garry_u32 page_size,
                                   garry_txn_id txn, const char *value, garry_i32 vlen);
garry_bool garry_chain_page_append_tombstone(garry_page_buffer buf, garry_u32 page_size,
                                             garry_txn_id txn);
garry_bool garry_chain_page_append_overflow(garry_page_buffer buf, garry_u32 page_size,
                                            garry_txn_id txn, garry_i32 total_len,
                                            garry_i32 head);
garry_i32 garry_overflow_chunk_size(garry_u32 page_size);
garry_i32 garry_chain_inline_capacity(garry_u32 page_size);
garry_i32 garry_overflow_write(garry_buffer_pool *pool, const char *value,
                               garry_i32 vlen);
garry_i32 garry_overflow_read(garry_buffer_pool *pool, garry_i32 head,
                              garry_i32 total_len, char *out_buf);
char* garry_chain_page_find_visible(garry_buffer_pool *pool,
                                    garry_page_buffer buf, garry_u32 page_size,
                                    garry_txn_id snap_txid,
                                    garry_txn_id_ptr active, garry_i32 active_count,
                                    garry_i32 *vlen);
void garry_chain_page_prune(garry_page_buffer buf, garry_u32 page_size,
                            garry_txn_id_ptr active, garry_i32 active_count);
garry_bool garry_chain_page_has_version(garry_page_buffer buf, garry_u32 page_size,
                                        garry_txn_id txn);

#endif /* GARRY_VERSION_CHAIN_H */
