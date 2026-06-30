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

/**
 * @brief Initialize a WAL log.
 *
 * Opens the log and checkpoint files, initializes the append mutex,
 * and sets the initial LSN to 0.
 *
 * @param wal             WAL log struct to initialize
 * @param log_path        Filesystem path for the WAL log file
 * @param checkpoint_path Filesystem path for the checkpoint file
 * @return GARRY_OK on success, or an error status on failure
 */
garry_status_t garry_wal_log_init(garry_wal_log *wal, const char *log_path,
                                  const char *checkpoint_path);

/**
 * @brief Append a record to the WAL log.
 *
 * Serializes the record to the on-disk format and writes it to the
 * log file. The append is mutex-serialized. Returns the new LSN.
 *
 * @param wal     WAL log to append to
 * @param record  WAL record to append
 * @return New log sequence number, or -1 on write failure
 */
garry_log_sequence_number garry_wal_log_append(garry_wal_log *wal, const garry_wal_record *record);

/**
 * @brief Flush the WAL log to durable storage.
 *
 * Calls fsync on the WAL log file to ensure all buffered writes
 * are persisted to disk.
 *
 * @param wal  WAL log to flush
 */
void garry_wal_log_flush(garry_wal_log *wal);

/**
 * @brief Close the WAL log and release resources.
 *
 * Flushes the log to durable storage, closes the file descriptor,
 * and destroys the append mutex.
 *
 * @param wal  WAL log to close
 */
void garry_wal_log_close(garry_wal_log *wal);

#endif /* GARRY_WAL_LOG_H */
