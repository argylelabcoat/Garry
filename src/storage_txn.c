/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_txn.c
 * @brief Transaction lifecycle management for the storage engine.
 *
 * Thin wrappers that delegate to the MVCC transaction subsystem,
 * providing a stable internal API for begin/commit/rollback.
 */

#include "storage_txn.h"
#include "wal_record.h"
#include "wal_log.h"

garry_txn_id garry_storage_begin(garry_engine_handle *eng)
{
    if (!eng) return -1;
    return garry_mvcc_begin(eng);
}

void garry_storage_commit(garry_engine_handle *eng, garry_txn_id txn)
{
    garry_wal_record *rec;

    if (!eng) return;

    rec = garry_make_commit_record(txn);
    if (rec) {
        garry_wal_log_append(&eng->wal, rec);
        garry_wal_record_free(rec);
        garry_wal_log_flush(&eng->wal);
    }

    garry_lock_release(&eng->lock_mgr, txn);
    garry_mvcc_commit(eng, txn);
}

void garry_storage_rollback(garry_engine_handle *eng, garry_txn_id txn)
{
    garry_wal_record *rec;

    if (!eng) return;

    rec = garry_make_abort_record(txn);
    if (rec) {
        garry_wal_log_append(&eng->wal, rec);
        garry_wal_record_free(rec);
    }

    garry_lock_release(&eng->lock_mgr, txn);
    garry_mvcc_rollback(eng, txn);
}
