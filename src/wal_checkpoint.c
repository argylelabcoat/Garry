/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file wal_checkpoint.c
 * @brief WAL checkpoint implementation.
 *
 * Flushes all dirty buffer pool pages to disk, syncs the file, then
 * truncates the WAL log. This reduces recovery time by limiting the
 * number of WAL records that need replay after a crash.
 */

#include "wal_checkpoint.h"
#include "gc.h"
#include "buffer_pool.h"
#include "version_chain.h"

#ifndef GARRY_TRUE
#define GARRY_TRUE 1
#endif
#ifndef GARRY_FALSE
#define GARRY_FALSE 0
#endif

garry_bool garry_wal_checkpoint(garry_wal_log *wal, garry_engine_handle *eng,
                                garry_log_sequence_number *checkpoint_lsn)
{
    if (!wal->fd.is_open) return GARRY_FALSE;

    *checkpoint_lsn = wal->last_lsn;
    garry_mvcc_gc(eng);
    garry_pool_flush_all(eng->pool);
    garry_wal_log_flush(wal);

    return GARRY_TRUE;
}
