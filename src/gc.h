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

/**
 * @brief Run MVCC garbage collection on version chain pages.
 *
 * Snapshots the set of active transactions, then iterates all pages
 * in the database. For each version chain page, prunes entries that
 * are no longer visible to any active transaction, reclaiming space.
 *
 * @param eng  Storage engine handle
 */
void garry_mvcc_gc(garry_engine_handle *eng);

#endif /* GARRY_GC_H */
