/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file lock_table.h
 * @brief Key-level lock manager for concurrency control.
 */

#ifndef GARRY_LOCK_TABLE_H
#define GARRY_LOCK_TABLE_H

#include "garry/types.h"
#include "storage_types.h"

typedef enum
{
    GARRY_LOCK_SHARED = 0,
    GARRY_LOCK_EXCLUSIVE
} garry_lock_mode;

typedef struct garry_lock_node
{
    garry_byte_array key;
    garry_i32 key_len;
    garry_txn_id txn;
    garry_lock_mode mode;
    struct garry_lock_node *next;
} garry_lock_node;

typedef struct
{
    garry_lock_node *head;
    garry_i32 count;
} garry_lock_manager;

/**
 * @brief Create and initialize a new lock manager.
 *
 * Allocates a lock manager with an empty lock table. The manager
 * tracks per-key shared/exclusive locks held by active transactions.
 *
 * @return Initialized lock manager (returned by value)
 */
garry_lock_manager garry_create_lock_manager(void);

/**
 * @brief Check if a transaction holds a lock on a specific key.
 *
 * Scans the lock list for an entry matching the given transaction ID
 * and key. Returns true if any lock (shared or exclusive) is held.
 *
 * @param mgr   Lock manager to query
 * @param txn   Transaction ID to check
 * @param key   Key bytes to check
 * @param klen  Key length in bytes
 * @return GARRY_TRUE if transaction holds a lock on this key
 */
garry_bool garry_lock_held(garry_lock_manager *mgr, garry_txn_id txn, const garry_byte *key,
                           garry_i32 klen);

/**
 * @brief Check if a lock request would conflict with existing locks.
 *
 * A conflict occurs when:
 * - Requesting EXCLUSIVE and any lock exists on the key, OR
 * - Requesting SHARED and an EXCLUSIVE lock exists on the key
 *
 * @param mgr   Lock manager to check
 * @param txn   Transaction ID requesting the lock
 * @param key   Key bytes to check
 * @param klen  Key length in bytes
 * @param mode  Requested lock mode (SHARED or EXCLUSIVE)
 * @return GARRY_TRUE if the request would conflict
 */
garry_bool garry_has_conflict(garry_lock_manager *mgr, garry_txn_id txn, const garry_byte *key,
                              garry_i32 klen, garry_lock_mode mode);

/**
 * @brief Acquire a lock on a key for a transaction.
 *
 * If no conflict exists, adds the lock to the manager's list.
 * If a conflict is detected, sets ok to GARRY_FALSE but does not
 * block — the caller must handle retry/backoff.
 *
 * @param mgr   Lock manager
 * @param txn   Transaction ID acquiring the lock
 * @param key   Key bytes to lock
 * @param klen  Key length in bytes
 * @param mode  Lock mode (SHARED or EXCLUSIVE)
 * @param ok    Output: GARRY_TRUE if lock acquired, GARRY_FALSE on conflict
 */
void garry_lock_acquire(garry_lock_manager *mgr, garry_txn_id txn, const garry_byte *key,
                        garry_i32 klen, garry_lock_mode mode, garry_bool *ok);

/**
 * @brief Release all locks held by a transaction.
 *
 * Removes all lock entries matching the given transaction ID from
 * the lock list. Called during transaction commit or abort.
 *
 * @param mgr  Lock manager
 * @param txn  Transaction ID whose locks to release
 */
void garry_lock_release(garry_lock_manager *mgr, garry_txn_id txn);

#endif /* GARRY_LOCK_TABLE_H */
