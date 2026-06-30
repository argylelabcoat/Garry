/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file wal_recovery.h
 * @brief WAL replay for crash recovery.
 */

#ifndef GARRY_WAL_RECOVERY_H
#define GARRY_WAL_RECOVERY_H

#include "transaction.h"
#include "wal_log.h"

/**
 * @brief Replay the WAL log to recover committed mutations.
 *
 * Performs a two-pass recovery: first collects all committed
 * transaction IDs, then replays update records for those transactions
 * into the B-tree. Uncommitted transactions are skipped.
 *
 * @param wal  WAL log to replay
 * @param eng  Storage engine to apply recovered mutations to
 * @return GARRY_TRUE on successful recovery, GARRY_FALSE on error
 */
garry_bool garry_wal_recover(garry_wal_log *wal, garry_engine_handle *eng);

#endif /* GARRY_WAL_RECOVERY_H */
