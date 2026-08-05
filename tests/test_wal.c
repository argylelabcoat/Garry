/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "wal_record.h"
#include "wal_log.h"
#include "version_chain.h"
#include "buffer_pool.h"
#include "file_io.h"
#include "db_header.h"
#include "test_helpers.h"
#include <string.h>
#include <stdlib.h>

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

    rec = garry_make_update_record(NULL, 1, key, 8, val, 8);
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

    rec = garry_make_update_record(NULL, 1, key, 4, val, 4);
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

/* Direct unit coverage for garry_make_update_record's overflow branch
 * (the fixed WAL heap-buffer-overflow bug): a value larger than a single
 * garry_byte_array (256 bytes) must be routed through the overflow-page
 * mechanism instead of being inlined, and must round-trip exactly via
 * garry_overflow_read. The small-value path must NOT set the overflow
 * flag, confirming the existing inline behavior is unchanged. */
static void test_wal_record_overflow_construction(void)
{
    garry_buffer_pool *pool;
    garry_wal_record *small_rec, *big_rec;
    garry_byte_array key;
    garry_u8 small_val[8];
    garry_u8 *big_val, *readback;
    garry_i32 big_len = 4096;
    garry_i32 i;
    garry_bool ok;

    remove("test_wal_overflow.db");
    pool = garry_pool_create("test_wal_overflow.db", 16, 4096);
    GARRY_CHECK(pool != NULL);

    memset(key, 0, sizeof(key));
    memcpy(key, "ovkey", 5);
    memset(small_val, 'Z', sizeof(small_val));

    small_rec = garry_make_update_record(pool, 1, key, 5, small_val, (garry_i32)sizeof(small_val));
    GARRY_CHECK(small_rec != NULL);
    GARRY_CHECK(small_rec->new_is_overflow == GARRY_FALSE);
    GARRY_CHECK(memcmp(small_rec->new_data, small_val, sizeof(small_val)) == 0);
    garry_wal_record_free(small_rec);

    big_val = (garry_u8 *)malloc((size_t)big_len);
    GARRY_CHECK(big_val != NULL);
    for (i = 0; i < big_len; i++)
        big_val[i] = (garry_u8)(i & 0xFF);

    big_rec = garry_make_update_record(pool, 2, key, 5, big_val, big_len);
    GARRY_CHECK(big_rec != NULL);
    GARRY_CHECK(big_rec->new_is_overflow == GARRY_TRUE);
    GARRY_CHECK(big_rec->new_overflow_head >= 0);
    GARRY_CHECK(big_rec->value_len == big_len);

    readback = (garry_u8 *)malloc((size_t)big_len);
    GARRY_CHECK(readback != NULL);
    ok = garry_overflow_read(pool, big_rec->new_overflow_head, big_len, (char *)readback);
    GARRY_CHECK(ok == GARRY_TRUE);
    GARRY_CHECK(memcmp(readback, big_val, (size_t)big_len) == 0);

    garry_wal_record_free(big_rec);
    free(big_val);
    free(readback);
    garry_pool_close(pool);
    remove("test_wal_overflow.db");
}

/* On-disk serialization round-trip for the new overflow flag/head fields:
 * append an overflow record, then read the raw bytes back off disk at the
 * fixed WAL_REC_NEW_OVERFLOW_* offsets (independent of wal_recovery.c's
 * replay logic) to confirm wal_log.c actually persists them correctly. */
static void test_wal_log_overflow_round_trip(void)
{
    garry_buffer_pool *pool;
    garry_wal_log wal;
    garry_status_t st;
    garry_wal_record *rec;
    garry_byte_array key;
    garry_u8 *big_val;
    garry_i32 big_len = 1024;
    garry_i32 i;
    garry_byte on_disk[GARRY_WAL_RECORD_SIZE];
    garry_file_descriptor fd;
    garry_i32 read_flag, read_head, read_vlen;

    remove("test_wal_ovf_log.db");
    remove("test_wal_ovf_log.log");
    pool = garry_pool_create("test_wal_ovf_log.db", 16, 4096);
    GARRY_CHECK(pool != NULL);

    memset(key, 0, sizeof(key));
    memcpy(key, "ovlogkey", 8);
    big_val = (garry_u8 *)malloc((size_t)big_len);
    GARRY_CHECK(big_val != NULL);
    for (i = 0; i < big_len; i++)
        big_val[i] = (garry_u8)('X');

    st = garry_wal_log_init(&wal, "test_wal_ovf_log.log", "test_wal_ovf_log.ckpt");
    GARRY_CHECK(st == GARRY_OK);

    rec = garry_make_update_record(pool, 9, key, 8, big_val, big_len);
    GARRY_CHECK(rec != NULL);
    GARRY_CHECK(rec->new_is_overflow == GARRY_TRUE);
    GARRY_CHECK(garry_wal_log_append(&wal, rec) == 1);
    garry_wal_log_flush(&wal);
    garry_wal_log_close(&wal);

    fd = garry_file_open("test_wal_ovf_log.log", GARRY_O_RDWR);
    GARRY_CHECK(fd.is_open);
    GARRY_CHECK(garry_file_read(&fd, on_disk, GARRY_WAL_RECORD_SIZE) == GARRY_WAL_RECORD_SIZE);
    garry_file_close(&fd);

    read_vlen = garry_read_int32(on_disk, WAL_REC_VLEN_OFF);
    read_flag = garry_read_int32(on_disk, WAL_REC_NEW_OVERFLOW_FLAG_OFF);
    read_head = garry_read_int32(on_disk, WAL_REC_NEW_OVERFLOW_HEAD_OFF);
    GARRY_CHECK(read_vlen == big_len);
    GARRY_CHECK(read_flag == 1);
    GARRY_CHECK(read_head == rec->new_overflow_head);

    garry_wal_record_free(rec);
    free(big_val);
    garry_pool_close(pool);
    garry_file_unlink("test_wal_ovf_log.log");
    remove("test_wal_ovf_log.db");
}

int main(void)
{
    test_wal_record_constructors();
    test_wal_log_io();
    test_wal_record_overflow_construction();
    test_wal_log_overflow_round_trip();

    if (garry_test_failures == 0)
        printf("test_wal: ALL PASSED\n");
    return garry_test_failures;
}
