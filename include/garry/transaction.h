/**
 * @file transaction.h
 * @brief Transaction lifecycle for the Garry storage engine.
 *
 * Provides begin/commit/rollback semantics that gate all read and write
 * operations. Every @ref garry_get, @ref garry_set, and @ref garry_delete
 * call requires an active transaction handle.
 *
 * Intent: Ensure serializable isolation between concurrent transactions
 * and provide atomic commit/rollback semantics.
 * Rationale: Transactions are the fundamental unit of consistency; without
 * them, partial writes could corrupt the on-disk B-tree.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_PUBLIC_TRANSACTION_H
#define GARRY_PUBLIC_TRANSACTION_H

#include "garry/export.h"
#include "garry/types.h"

/// Forward declaration — defined in database.h.
typedef struct garry_database garry_database;
/// Transaction identifier — an opaque handle returned by @ref garry_txn_begin.
typedef garry_i32 garry_txn;

/**
 * @brief Begin a new transaction on the database.
 * @param db  Database handle.
 * @return Transaction handle that must be passed to commit or rollback.
 *
 * Acquires a write lock on the database. Only one write transaction may
 * be active at a time; calling @ref garry_txn_begin while another
 * transaction is active will block until it completes.
 */
GARRY_API garry_txn garry_txn_begin(garry_database *db);

/**
 * @brief Commit all changes made within a transaction.
 * @param db   Database handle.
 * @param txn  Transaction handle returned by @ref garry_txn_begin.
 *
 * Flushes all pending mutations to disk. After this call @p txn is invalid.
 * If the process crashes before commit, all changes in @p txn are lost.
 */
GARRY_API void garry_txn_commit(garry_database *db, garry_txn txn);

/**
 * @brief Discard all changes made within a transaction.
 * @param db   Database handle.
 * @param txn  Transaction handle returned by @ref garry_txn_begin.
 *
 * Reverts the database to the state it was in before @ref garry_txn_begin
 * was called. After this call @p txn is invalid.
 */
GARRY_API void garry_txn_rollback(garry_database *db, garry_txn txn);

#endif /* GARRY_PUBLIC_TRANSACTION_H */
