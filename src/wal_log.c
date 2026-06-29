/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file wal_log.c
 * @brief Write-ahead log implementation.
 *
 * The WAL is an append-only file that records all mutations before
 * they are applied to the main database. Each record is serialized
 * with a header, and the log maintains the current LSN (log sequence
 * number). fsync is called on flush to guarantee durability.
 */

#include "wal_log.h"
#include <string.h>

garry_status_t garry_wal_log_init(garry_wal_log *wal, const char *log_path,
                                  const char *checkpoint_path)
{
    garry_file_descriptor fd;
    if (log_path == NULL || checkpoint_path == NULL)
    {
        return GARRY_ERR_INVALID_ARG;
    }
    fd = garry_file_open(log_path, GARRY_O_RDWR | GARRY_O_CREAT);
    if (!fd.is_open)
    {
        return GARRY_ERR_IO;
    }
    wal->fd = fd;
    wal->last_lsn = 0;
    wal->path = log_path;
    wal->ckpt_path = checkpoint_path;
    garry_mutex_init(&wal->append_mutex);
    return GARRY_OK;
}

static garry_bool wal_write_record(garry_wal_log *wal, const garry_wal_record *rec)
{
    garry_byte buf[GARRY_WAL_RECORD_SIZE];
    garry_i32 kind = (garry_i32)rec->kind;
    garry_i32 txid = rec->txid;
    garry_i32 n;

    memset(buf, 0, sizeof(buf));
    memcpy(buf + WAL_REC_KIND_OFF, &kind, sizeof(garry_i32));
    memcpy(buf + WAL_REC_TXID_OFF, &txid, sizeof(garry_i32));
    memcpy(buf + WAL_REC_KLEN_OFF, &rec->key_len, sizeof(garry_i32));
    memcpy(buf + WAL_REC_VLEN_OFF, &rec->value_len, sizeof(garry_i32));
    memcpy(buf + WAL_REC_KEY_OFF, rec->key, sizeof(garry_byte_array));
    memcpy(buf + WAL_REC_OLD_OFF, rec->old_data, sizeof(garry_byte_array));
    memcpy(buf + WAL_REC_NEW_OFF, rec->new_data, sizeof(garry_byte_array));

    n = garry_file_write_bytes(&wal->fd, buf, GARRY_WAL_RECORD_SIZE);
    return (n == GARRY_WAL_RECORD_SIZE) ? GARRY_TRUE : GARRY_FALSE;
}

garry_log_sequence_number garry_wal_log_append(garry_wal_log *wal, const garry_wal_record *record)
{
    garry_log_sequence_number lsn;
    if (!wal->fd.is_open)
        return -1;
    garry_mutex_lock(&wal->append_mutex);
    lsn = wal->last_lsn + 1;
    if (!wal_write_record(wal, record))
    {
        garry_mutex_unlock(&wal->append_mutex);
        return -1;
    }
    wal->last_lsn = lsn;
    garry_mutex_unlock(&wal->append_mutex);
    return lsn;
}

void garry_wal_log_flush(garry_wal_log *wal)
{
    if (!wal->fd.is_open)
        return;
    garry_file_sync(&wal->fd);
}

void garry_wal_log_close(garry_wal_log *wal)
{
    if (!wal->fd.is_open)
        return;
    garry_file_sync(&wal->fd);
    garry_file_close(&wal->fd);
    garry_mutex_destroy(&wal->append_mutex);
}
