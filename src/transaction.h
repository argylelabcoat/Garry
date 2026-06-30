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

typedef enum
{
    GARRY_TXN_ACTIVE = 0,
    GARRY_TXN_COMMITTED,
    GARRY_TXN_ROLLED_BACK
} garry_txn_state;

typedef struct
{
    garry_txn_id txid;
    garry_txn_state state;
    garry_i32 modified_count;
    garry_i32 modified_pages[GARRY_MAX_MODIFIED_PAGES];
} garry_txn_info;

typedef struct
{
    garry_i32 page_size;
    garry_i32 compression;
    garry_i32 btree_root;
    garry_buffer_pool *pool;
    garry_txn_id next_txid;
    garry_i32 max_txns;
    garry_txn_id *active_txns;
    garry_i32 active_count;
    garry_txn_info *txn_states;
    garry_wal_log wal;
    garry_lock_manager lock_mgr;
    garry_engine_settings settings;
    garry_db_header header;
    garry_rwlock root_lock;
    garry_mutex txn_slot_mutex;
    garry_i32 key_count;
} garry_engine_handle;

/**
 * Initialise a new Garry storage engine, creating the database file.
 *
 * Allocates the engine handle, buffer pool, WAL log, lock manager, and
 * writes the initial DB header and empty B-tree root page.
 *
 * @param path Filesystem path of the database file to create.
 * @param settings Engine configuration (page size, pool size, max txns, etc.).
 * @return Engine handle, or NULL on allocation or I/O failure.
 */
garry_engine_handle *garry_engine_init(const char *path, garry_engine_settings settings);
/**
 * Open an existing database.
 *
 * IMPORTANT: Garry databases are not safe for concurrent access from
 * multiple processes. Only one process should open a database at a time.
 * Cross-process access may cause data corruption.
 *
 * @param path Filesystem path of the database file.
 * @return Engine handle, or NULL on failure.
 */
garry_engine_handle *garry_engine_open(const char *path);
/**
 * Close an engine handle and release all resources.
 *
 * Persists free-list state to the DB header, flushes all dirty pages,
 * closes the WAL log and file descriptor, and frees all memory.
 *
 * @param eng Engine handle to close. NULL is safely ignored.
 */
void garry_engine_close(garry_engine_handle *eng);

/**
 * Encode a chain page ID as 4 little-endian bytes.
 *
 * @param chain_id The chain page ID to encode.
 * @param out Output buffer receiving the 4 encoded bytes.
 * @return Number of bytes written (always 4).
 */
garry_i32 garry_chain_id_encode(garry_i32 chain_id, garry_byte *out);

/**
 * Decode a chain page ID from 4 little-endian bytes.
 *
 * @param encoded Pointer to the 4-byte encoded chain ID.
 * @return Decoded chain page ID.
 */
garry_i32 garry_chain_id_decode(const garry_byte *encoded);

/**
 * Allocate and initialise a new version chain page.
 *
 * @param eng Engine handle owning the buffer pool.
 * @param key Key associated with the chain (reserved for future use).
 * @param klen Length of the key in bytes.
 * @return Page ID of the newly allocated chain page, or -1 on failure.
 */
garry_i32 garry_chain_allocate(garry_engine_handle *eng, const garry_byte *key, garry_i32 klen);

/**
 * Begin a new MVCC transaction.
 *
 * Allocates a transaction slot and returns a monotonically increasing
 * transaction ID. Fails if the maximum number of concurrent transactions
 * has been reached.
 *
 * @param eng Engine handle.
 * @return Transaction ID (> 0), or -1 if no slots are available.
 */
garry_txn_id garry_mvcc_begin(garry_engine_handle *eng);

/**
 * Commit an active transaction.
 *
 * Marks the transaction as committed, releases its locks, and flushes
 * any modified pages to disk.
 *
 * @param eng Engine handle.
 * @param txn Transaction ID to commit.
 */
void garry_mvcc_commit(garry_engine_handle *eng, garry_txn_id txn);

/**
 * Roll back an active transaction.
 *
 * Marks the transaction as rolled back and releases its locks. Version
 * chain entries written by this transaction become invisible to future
 * snapshots.
 *
 * @param eng Engine handle.
 * @param txn Transaction ID to roll back.
 */
void garry_mvcc_rollback(garry_engine_handle *eng, garry_txn_id txn);

/**
 * Check whether a transaction is still active.
 *
 * @param txn Transaction ID to query.
 * @param eng Engine handle.
 * @return GARRY_TRUE if the transaction is active, GARRY_FALSE otherwise.
 */
garry_bool garry_txn_is_active(garry_txn_id txn, garry_engine_handle *eng);

/**
 * Read the visible version of a key's value under MVCC snapshot isolation.
 *
 * @param eng Engine handle.
 * @param txn Transaction ID acting as the snapshot.
 * @param chain_page_id Page ID of the version chain to search.
 * @param vlen Output parameter receiving the value length in bytes.
 * @return Malloc'd copy of the visible value, or NULL if not found or deleted.
 */
char *garry_mvcc_get(garry_engine_handle *eng, garry_txn_id txn, garry_i32 chain_page_id,
                     garry_i32 *vlen);

/**
 * Append a new version entry to a key's version chain.
 *
 * If the value exceeds the inline capacity, it is written to overflow
 * pages and an overflow reference is stored in the chain entry.
 *
 * @param eng Engine handle.
 * @param txn Transaction ID performing the write.
 * @param chain_page_id Page ID of the version chain.
 * @param value Value bytes to store.
 * @param vlen Length of the value in bytes.
 * @return GARRY_TRUE on success, GARRY_FALSE on failure.
 */
garry_bool garry_mvcc_set(garry_engine_handle *eng, garry_txn_id txn, garry_i32 chain_page_id,
                           const char *value, garry_i32 vlen);

/**
 * Append a tombstone entry to a key's version chain.
 *
 * The tombstone marks the key as deleted for MVCC readers. Existing
 * overflow pages referenced by prior versions are not freed.
 *
 * @param eng Engine handle.
 * @param txn Transaction ID performing the deletion.
 * @param chain_page_id Page ID of the version chain.
 * @return GARRY_TRUE on success, GARRY_FALSE on failure.
 */
garry_bool garry_mvcc_delete(garry_engine_handle *eng, garry_txn_id txn, garry_i32 chain_page_id);

/**
 * Write a value to overflow pages for storage in a version chain entry.
 *
 * This is a low-level helper used when the value exceeds inline capacity.
 * It does not append the chain entry itself; use garry_mvcc_set instead.
 *
 * @param eng Engine handle.
 * @param txn Transaction ID (reserved for future use).
 * @param value Value bytes to write.
 * @param vlen Length of the value in bytes.
 * @return Head page ID of the overflow chain, or -1 on failure.
 */
garry_i32 garry_mvcc_chain_overflow(garry_engine_handle *eng, garry_txn_id txn, const char *value,
                                    garry_i32 vlen);

/**
 * Apply a recovered value to a version chain during WAL replay.
 *
 * Delegates to garry_mvcc_set to append the value under the given
 * transaction ID.
 *
 * @param eng Engine handle.
 * @param chain_page_id Page ID of the version chain.
 * @param txn Transaction ID from the WAL record.
 * @param value Value bytes to apply.
 * @param vlen Length of the value in bytes.
 * @return GARRY_TRUE on success, GARRY_FALSE on failure.
 */
garry_bool garry_mvcc_recovery_apply(garry_engine_handle *eng, garry_i32 chain_page_id,
                                     garry_txn_id txn, const char *value, garry_i32 vlen);

#endif /* GARRY_TRANSACTION_H */
