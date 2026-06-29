/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file wal_log.h
 * @brief Write-ahead log: append-only journal for crash recovery.
 */

#ifndef GARRY_WAL_LOG_H
#define GARRY_WAL_LOG_H

#include "garry/types.h"
#include "storage_types.h"
#include "file_io.h"
#include "wal_record.h"
#include "garry_threading.h"

typedef garry_i32 garry_log_sequence_number;

typedef struct
{
    garry_file_descriptor fd;
    garry_log_sequence_number last_lsn;
    const char *path;
    const char *ckpt_path;
    garry_mutex append_mutex;
} garry_wal_log;

/* WAL_INIT(log_path, checkpoint_path) RET wal_handle */
garry_status_t garry_wal_log_init(garry_wal_log *wal, const char *log_path,
                                  const char *checkpoint_path);

/* WAL_APPEND(wal, record) RET log_sequence_number */
garry_log_sequence_number garry_wal_log_append(garry_wal_log *wal, const garry_wal_record *record);

/* WAL_FLUSH(wal) */
void garry_wal_log_flush(garry_wal_log *wal);

/* WAL_CLOSE(wal) */
void garry_wal_log_close(garry_wal_log *wal);

#endif /* GARRY_WAL_LOG_H */
