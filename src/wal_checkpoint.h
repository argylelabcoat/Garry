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

garry_bool garry_wal_checkpoint(garry_wal_log *wal, garry_engine_handle *eng,
                                garry_log_sequence_number *checkpoint_lsn);

#endif /* GARRY_WAL_CHECKPOINT_H */
