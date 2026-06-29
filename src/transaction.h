/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file transaction.h
 * @brief MVCC transaction manager and engine handle definition.
 *
 * Defines the garry_engine_handle struct (the central state object of the
 * storage engine) and provides begin/commit/rollback operations with
 * multi-version concurrency control.
 */

#ifndef GARRY_TRANSACTION_H
#define GARRY_TRANSACTION_H

#include "storage_types.h"
#include "storage_core.h"
#include "db_header.h"
#include "engine_settings.h"
#include "buffer_pool.h"
#include "version_chain.h"
#include "wal_log.h"
#include "lock_table.h"
#include "garry_threading.h"

#define GARRY_MAX_MODIFIED_PAGES 16

typedef enum {
    GARRY_TXN_ACTIVE = 0,
    GARRY_TXN_COMMITTED,
    GARRY_TXN_ROLLED_BACK
} garry_txn_state;

typedef struct {
    garry_txn_id txid;
    garry_txn_state state;
    garry_i32 modified_count;
} garry_txn_info;

typedef struct {
    garry_i32 page_size;
    garry_i32 compression;
    garry_i32 btree_root;
    garry_buffer_pool *pool;
    garry_txn_id next_txid;
    garry_i32 max_txns;
    garry_txn_id *active_txns;
    garry_i32 active_count;
    garry_txn_info *txn_states;
    garry_i32 *modified_pages;
    garry_wal_log wal;
    garry_lock_manager lock_mgr;
    garry_engine_settings settings;
    garry_db_header header;
    garry_rwlock root_lock;
    garry_mutex txn_slot_mutex;
} garry_engine_handle;

garry_engine_handle* garry_engine_init(const char *path, garry_engine_settings settings);
garry_engine_handle* garry_engine_open(const char *path);
void garry_engine_close(garry_engine_handle *eng);

garry_i32 garry_chain_id_encode(garry_i32 chain_id, garry_byte *out);
garry_i32 garry_chain_id_decode(const garry_byte *encoded);
garry_i32 garry_chain_allocate(garry_engine_handle *eng, const garry_byte *key, garry_i32 klen);

garry_txn_id garry_mvcc_begin(garry_engine_handle *eng);
void garry_mvcc_commit(garry_engine_handle *eng, garry_txn_id txn);
void garry_mvcc_rollback(garry_engine_handle *eng, garry_txn_id txn);
garry_bool garry_txn_is_active(garry_txn_id txn, garry_engine_handle *eng);

char* garry_mvcc_get(garry_engine_handle *eng, garry_txn_id txn,
                     garry_i32 chain_page_id, garry_i32 *vlen);
garry_bool garry_mvcc_set(garry_engine_handle *eng, garry_txn_id txn,
                          garry_i32 chain_page_id, const char *value, garry_i32 vlen);
garry_bool garry_mvcc_delete(garry_engine_handle *eng, garry_txn_id txn,
                             garry_i32 chain_page_id);
garry_i32 garry_mvcc_chain_overflow(garry_engine_handle *eng, garry_txn_id txn,
                                    const char *value, garry_i32 vlen);
garry_bool garry_mvcc_recovery_apply(garry_engine_handle *eng, garry_i32 chain_page_id,
                                     garry_txn_id txn, const char *value, garry_i32 vlen);

#endif /* GARRY_TRANSACTION_H */
