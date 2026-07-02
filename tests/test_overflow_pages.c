/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_overflow_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

/* Value that fits inline (no overflow). */
static void test_inline_value(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[16], value[64], result[256];
    garry_i32 vlen;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    memset(value, 'A', 64);
    garry_set(db, txn, (garry_u8 *)"small", 5, value, 64);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    GARRY_CHECK(garry_get(db, txn, (garry_u8 *)"small", 5, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 64);
    GARRY_CHECK(result[0] == 'A');
    GARRY_CHECK(result[63] == 'A');
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

/* Value that exceeds inline capacity — forces overflow pages. */
static void test_overflow_value(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[16], value[4096], result[4096];
    garry_i32 vlen;
    int i;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    for (i = 0; i < 4096; i++)
        value[i] = (garry_u8)('A' + (i % 26));
    garry_set(db, txn, (garry_u8 *)"big", 3, value, 4096);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    GARRY_CHECK(garry_get(db, txn, (garry_u8 *)"big", 3, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 4096);
    GARRY_CHECK(result[0] == 'A');
    GARRY_CHECK(result[1] == 'B');
    GARRY_CHECK(result[25] == 'Z');
    GARRY_CHECK(result[26] == 'A');
    GARRY_CHECK(result[4095] == (garry_u8)('A' + (4095 % 26)));
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

/* Very large value — multiple overflow pages. */
static void test_large_overflow(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[16], result[16384];
    garry_u8 *value;
    garry_i32 vlen;
    int i;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    value = (garry_u8 *)malloc(16384);
    GARRY_CHECK(value != NULL);

    txn = garry_txn_begin(db);
    for (i = 0; i < 16384; i++)
        value[i] = (garry_u8)(i & 0xFF);
    garry_set(db, txn, (garry_u8 *)"huge", 4, value, 16384);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    GARRY_CHECK(garry_get(db, txn, (garry_u8 *)"huge", 4, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 16384);
    GARRY_CHECK(result[0] == 0);
    GARRY_CHECK(result[1] == 1);
    GARRY_CHECK(result[255] == 255);
    GARRY_CHECK(result[256] == 0);
    garry_txn_commit(db, txn);

    free(value);
    garry_database_close(db);
    cleanup();
}

/* Overwrite overflow value with smaller one. */
static void test_overflow_overwrite(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[16], value[4096], result[4096];
    garry_i32 vlen;
    int i;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    /* Write large value */
    txn = garry_txn_begin(db);
    for (i = 0; i < 4096; i++)
        value[i] = 'X';
    garry_set(db, txn, (garry_u8 *)"ow", 2, value, 4096);
    garry_txn_commit(db, txn);

    /* Overwrite with small value */
    txn = garry_txn_begin(db);
    garry_set(db, txn, (garry_u8 *)"ow", 2, (garry_u8 *)"tiny", 4);
    garry_txn_commit(db, txn);

    /* Read — must get small value */
    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    GARRY_CHECK(garry_get(db, txn, (garry_u8 *)"ow", 2, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 4);
    GARRY_CHECK(memcmp(result, "tiny", 4) == 0);
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

/* Multiple overflow keys. */
static void test_multiple_overflow_keys(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 value[4096], result[4096];
    garry_i32 vlen;
    int i, j;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    for (i = 0; i < 5; i++)
    {
        char kbuf[16];
        sprintf(kbuf, "big%d", i);
        for (j = 0; j < 4096; j++)
            value[j] = (garry_u8)('A' + i);
        garry_set(db, txn, (garry_u8 *)kbuf, (garry_i32)strlen(kbuf), value, 4096);
    }
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    for (i = 0; i < 5; i++)
    {
        char kbuf[16];
        sprintf(kbuf, "big%d", i);
        memset(result, 0, sizeof(result));
        GARRY_CHECK(garry_get(db, txn, (garry_u8 *)kbuf, (garry_i32)strlen(kbuf), result, &vlen) == GARRY_OK);
        GARRY_CHECK(vlen == 4096);
        GARRY_CHECK(result[0] == (garry_u8)('A' + i));
    }
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

/* Delete overflow value. */
static void test_delete_overflow(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 value[4096], result[4096];
    garry_i32 vlen;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    memset(value, 'Z', 4096);
    garry_set(db, txn, (garry_u8 *)"dovf", 4, value, 4096);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    GARRY_CHECK(garry_delete(db, txn, (garry_u8 *)"dovf", 4) == GARRY_OK);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    GARRY_CHECK(garry_get(db, txn, (garry_u8 *)"dovf", 4, result, &vlen) == GARRY_ERR_NOT_FOUND);
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

int main(void)
{
    test_inline_value();
    test_overflow_value();
    test_large_overflow();
    test_overflow_overwrite();
    test_multiple_overflow_keys();
    test_delete_overflow();
    if (garry_test_failures == 0)
        printf("test_overflow_pages: ALL PASSED\n");
    return garry_test_failures;
}
