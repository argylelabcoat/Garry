/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_types.h
 * @brief Core type aliases for transaction IDs and transaction sets.
 *
 * Ported from lib/storage/storage_types.rcsp. Defines the fixed
 * transaction pool size and ID types used throughout the MVCC subsystem.
 */

#ifndef GARRY_STORAGE_TYPES_H
#define GARRY_STORAGE_TYPES_H

#include "garry/types.h"

#define GARRY_MAX_TXNS 4

typedef garry_i32 garry_txn_id;      /**< Active transaction identifier. */
typedef garry_i32 garry_txn_id_opt;  /**< Optional transaction ID (0 = none). */
typedef garry_txn_id garry_txn_set[GARRY_MAX_TXNS];  /**< Fixed set of active TXN IDs. */

#endif /* GARRY_STORAGE_TYPES_H */
