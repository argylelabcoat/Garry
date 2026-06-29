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

garry_bool garry_wal_recover(garry_wal_log *wal, garry_engine_handle *eng);

#endif /* GARRY_WAL_RECOVERY_H */
