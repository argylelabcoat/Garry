/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file wal_checkpoint.h
 * @brief WAL checkpoint: flush dirty pages and truncate the log.
 */

#ifndef GARRY_WAL_CHECKPOINT_H
#define GARRY_WAL_CHECKPOINT_H

#include "transaction.h"
#include "wal_log.h"

/**
 * @brief Perform a WAL checkpoint: flush dirty pages and truncate the log.
 *
 * Records the current LSN, runs MVCC garbage collection, flushes all
 * dirty buffer pool pages to disk, syncs the WAL, and truncates the
 * log file. This limits the work needed for crash recovery.
 *
 * @param wal            WAL log to checkpoint
 * @param eng            Storage engine handle
 * @param checkpoint_lsn Output: LSN at the time of the checkpoint
 * @return GARRY_TRUE on success, GARRY_FALSE on error
 */
garry_bool garry_wal_checkpoint(garry_wal_log *wal, garry_engine_handle *eng,
                                garry_log_sequence_number *checkpoint_lsn);

#endif /* GARRY_WAL_CHECKPOINT_H */
