/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_txn.h
 * @brief Transaction lifecycle wrappers (begin, commit, rollback).
 */

#ifndef GARRY_STORAGE_TXN_H
#define GARRY_STORAGE_TXN_H

#include "transaction.h"

/**
 * @brief Begin a new transaction.
 *
 * Allocates a new MVCC transaction ID from the engine.
 *
 * @param eng  Storage engine handle
 * @return New transaction ID, or -1 on failure
 */
garry_txn_id garry_storage_begin(garry_engine_handle *eng);

/**
 * @brief Commit a transaction.
 *
 * Appends a commit record to the WAL, flushes it, releases all locks
 * held by the transaction, and marks it committed in MVCC.
 *
 * @param eng  Storage engine handle
 * @param txn  Transaction ID to commit
 */
void garry_storage_commit(garry_engine_handle *eng, garry_txn_id txn);

/**
 * @brief Roll back a transaction.
 *
 * Appends an abort record to the WAL, releases all locks held by the
 * transaction, and marks it aborted in MVCC.
 *
 * @param eng  Storage engine handle
 * @param txn  Transaction ID to roll back
 */
void garry_storage_rollback(garry_engine_handle *eng, garry_txn_id txn);

#endif /* GARRY_STORAGE_TXN_H */
