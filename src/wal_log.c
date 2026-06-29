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

#ifndef GARRY_TRUE
#define GARRY_TRUE  1
#endif
#ifndef GARRY_FALSE
#define GARRY_FALSE 0
#endif

garry_status_t garry_wal_log_init(garry_wal_log* wal, const char* log_path,
                                  const char* checkpoint_path)
{
    garry_file_descriptor fd;
    if (log_path == NULL || checkpoint_path == NULL) {
        return GARRY_ERR_INVALID_ARG;
    }
    fd = garry_file_open(log_path, GARRY_O_RDWR | GARRY_O_CREAT);
    if (!fd.is_open) {
        return GARRY_ERR_IO;
    }
    wal->fd = fd;
    wal->last_lsn = 0;
    wal->path = log_path;
    wal->ckpt_path = checkpoint_path;
    garry_mutex_init(&wal->append_mutex);
    return GARRY_OK;
}

static garry_bool wal_write_record(garry_wal_log* wal, const garry_wal_record* rec)
{
    garry_i32 kind = (garry_i32)rec->kind;
    garry_i32 txid = rec->txid;
    garry_i32 klen = rec->key_len;
    garry_i32 vlen = rec->value_len;
    garry_i32 n;

    n = garry_file_write_bytes(&wal->fd, (const garry_byte*)&kind, (garry_i32)sizeof(garry_i32));
    if (n != (garry_i32)sizeof(garry_i32)) return GARRY_FALSE;
    n = garry_file_write_bytes(&wal->fd, (const garry_byte*)&txid, (garry_i32)sizeof(garry_i32));
    if (n != (garry_i32)sizeof(garry_i32)) return GARRY_FALSE;
    n = garry_file_write_bytes(&wal->fd, (const garry_byte*)&klen, (garry_i32)sizeof(garry_i32));
    if (n != (garry_i32)sizeof(garry_i32)) return GARRY_FALSE;
    n = garry_file_write_bytes(&wal->fd, (const garry_byte*)&vlen, (garry_i32)sizeof(garry_i32));
    if (n != (garry_i32)sizeof(garry_i32)) return GARRY_FALSE;
    n = garry_file_write_bytes(&wal->fd, rec->key, (garry_i32)sizeof(garry_byte_array));
    if (n != (garry_i32)sizeof(garry_byte_array)) return GARRY_FALSE;
    n = garry_file_write_bytes(&wal->fd, rec->old_data, (garry_i32)sizeof(garry_byte_array));
    if (n != (garry_i32)sizeof(garry_byte_array)) return GARRY_FALSE;
    n = garry_file_write_bytes(&wal->fd, rec->new_data, (garry_i32)sizeof(garry_byte_array));
    if (n != (garry_i32)sizeof(garry_byte_array)) return GARRY_FALSE;
    return GARRY_TRUE;
}

garry_log_sequence_number garry_wal_log_append(garry_wal_log* wal,
                                               const garry_wal_record* record)
{
    garry_log_sequence_number lsn;
    if (!wal->fd.is_open) return -1;
    garry_mutex_lock(&wal->append_mutex);
    lsn = wal->last_lsn + 1;
    if (!wal_write_record(wal, record)) {
        garry_mutex_unlock(&wal->append_mutex);
        return -1;
    }
    wal->last_lsn = lsn;
    garry_mutex_unlock(&wal->append_mutex);
    return lsn;
}

void garry_wal_log_flush(garry_wal_log* wal)
{
    if (!wal->fd.is_open) return;
    garry_file_sync(&wal->fd);
}

void garry_wal_log_close(garry_wal_log* wal)
{
    if (!wal->fd.is_open) return;
    garry_file_sync(&wal->fd);
    garry_file_close(&wal->fd);
    garry_mutex_destroy(&wal->append_mutex);
}
