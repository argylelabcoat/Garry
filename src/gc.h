/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file gc.h
 * @brief MVCC garbage collection for old version chains.
 */

#ifndef GARRY_GC_H
#define GARRY_GC_H

#include "transaction.h"

garry_txn_id garry_oldest_active_txid(garry_engine_handle *eng);
void garry_mvcc_gc(garry_engine_handle *eng);

#endif /* GARRY_GC_H */
