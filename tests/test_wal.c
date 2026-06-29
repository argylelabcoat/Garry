/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "wal_record.h"
#include "wal_log.h"
#include "test_helpers.h"
#include <string.h>

static void test_wal_record_constructors(void)
{
    garry_byte_array key, val;
    garry_wal_record *rec;
    garry_i32 i;

    memset(key, 0, sizeof(key));
    memset(val, 0, sizeof(val));
    for (i = 0; i < 8; i++)
    {
        key[i] = (garry_byte)(97 + i);
        val[i] = (garry_byte)(65 + i);
    }

    rec = garry_make_update_record(1, key, 8, val, 8);
    GARRY_CHECK(rec != NULL);
    GARRY_CHECK(rec->kind == GARRY_WAL_UPDATE);
    GARRY_CHECK(rec->txid == 1);
    GARRY_CHECK(rec->key_len == 8);
    for (i = 0; i < 8; i++)
    {
        GARRY_CHECK(rec->key[i] == key[i]);
        GARRY_CHECK(rec->new_data[i] == val[i]);
    }
    garry_wal_record_free(rec);

    rec = garry_make_commit_record(2);
    GARRY_CHECK(rec != NULL);
    GARRY_CHECK(rec->kind == GARRY_WAL_COMMIT);
    GARRY_CHECK(rec->txid == 2);
    garry_wal_record_free(rec);

    rec = garry_make_abort_record(3);
    GARRY_CHECK(rec != NULL);
    GARRY_CHECK(rec->kind == GARRY_WAL_ABORT);
    GARRY_CHECK(rec->txid == 3);
    garry_wal_record_free(rec);

    rec = garry_make_checkpoint_record(4);
    GARRY_CHECK(rec != NULL);
    GARRY_CHECK(rec->kind == GARRY_WAL_CHECKPOINT);
    GARRY_CHECK(rec->txid == 4);
    garry_wal_record_free(rec);
}

static void test_wal_log_io(void)
{
    garry_wal_log wal;
    garry_status_t st;
    garry_wal_record *rec;
    garry_log_sequence_number lsn1, lsn2;
    garry_byte_array key, val;
    garry_i32 i;

    memset(key, 0, sizeof(key));
    memset(val, 0, sizeof(val));
    for (i = 0; i < 4; i++)
    {
        key[i] = (garry_byte)(48 + i);
        val[i] = (garry_byte)(65 + i);
    }

    st = garry_wal_log_init(&wal, "test_wal.log", "test_wal.ckpt");
    GARRY_CHECK(st == GARRY_OK);
    GARRY_CHECK(wal.fd.is_open);
    GARRY_CHECK(wal.last_lsn == 0);

    rec = garry_make_update_record(1, key, 4, val, 4);
    lsn1 = garry_wal_log_append(&wal, rec);
    GARRY_CHECK(lsn1 == 1);
    GARRY_CHECK(wal.last_lsn == 1);
    garry_wal_record_free(rec);

    rec = garry_make_commit_record(1);
    lsn2 = garry_wal_log_append(&wal, rec);
    GARRY_CHECK(lsn2 == 2);
    GARRY_CHECK(wal.last_lsn == 2);
    garry_wal_record_free(rec);

    garry_wal_log_flush(&wal);
    garry_wal_log_close(&wal);
    GARRY_CHECK(!wal.fd.is_open);

    garry_file_unlink("test_wal.log");
}

int main(void)
{
    test_wal_record_constructors();
    test_wal_log_io();

    if (garry_test_failures == 0)
        printf("test_wal: ALL PASSED\n");
    return garry_test_failures;
}
