/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file test_wal_large_values.c
 * @brief Tests for WAL record handling with large values.
 *
 * Tests that the WAL record allocator correctly handles values of various
 * sizes, including values that exceed the original 256-byte inline buffer.
 */

#include "garry/api.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_wal_large_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

/**
 * @brief Test storing a value at the exact inline limit (256 bytes).
 */
static void test_value_at_256_bytes(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64];
    garry_u8 *value, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 val_size = 256;
    garry_i32 i;

    printf("test_value_at_256_bytes\n");
    cleanup();

    value = (garry_u8 *)malloc(val_size);
    result = (garry_u8 *)malloc(val_size);
    GARRY_CHECK(value != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    key[0] = 'k';
    key[1] = '2';
    key[2] = '5';
    key[3] = '6';
    for (i = 0; i < val_size; i++)
        value[i] = (garry_u8)('A' + (i % 26));

    ok = garry_set(db, txn, key, 4, value, val_size);
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, val_size);
    vlen = 0;
    ok = garry_get(db, txn, key, 4, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == val_size);
    GARRY_CHECK(result[0] == 'A');
    GARRY_CHECK(result[val_size - 1] == (garry_u8)('A' + ((val_size - 1) % 26)));

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value);
    free(result);
}

/**
 * @brief Test storing a value just over the inline limit (257 bytes).
 * Note: This test verifies that values larger than the WAL record inline
 * buffer are handled correctly via overflow pages.
 */
static void test_value_at_257_bytes(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64];
    garry_u8 *value, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 val_size = 257;
    garry_i32 i;

    printf("test_value_at_257_bytes\n");
    cleanup();

    value = (garry_u8 *)malloc(val_size);
    result = (garry_u8 *)malloc(val_size);
    GARRY_CHECK(value != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    key[0] = 'k';
    key[1] = '2';
    key[2] = '5';
    key[3] = '7';
    for (i = 0; i < val_size; i++)
        value[i] = (garry_u8)('A' + (i % 26));

    ok = garry_set(db, txn, key, 4, value, val_size);
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, val_size);
    vlen = 0;
    ok = garry_get(db, txn, key, 4, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == val_size);
    GARRY_CHECK(result[0] == 'A');
    GARRY_CHECK(result[val_size - 1] == (garry_u8)('A' + ((val_size - 1) % 26)));

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value);
    free(result);
}

/**
 * @brief Test storing a value at 1KB.
 * Note: This tests the overflow page mechanism for values larger than
 * the WAL record inline buffer (256 bytes).
 */
static void test_value_at_1kb(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64];
    garry_u8 *value, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 val_size = 1024;
    garry_i32 i;

    printf("test_value_at_1kb\n");
    cleanup();

    value = (garry_u8 *)malloc(val_size);
    result = (garry_u8 *)malloc(val_size);
    GARRY_CHECK(value != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    key[0] = 'k';
    key[1] = '1';
    key[2] = 'k';
    for (i = 0; i < val_size; i++)
        value[i] = (garry_u8)('A' + (i % 26));

    ok = garry_set(db, txn, key, 3, value, val_size);
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, val_size);
    vlen = 0;
    ok = garry_get(db, txn, key, 3, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == val_size);
    GARRY_CHECK(result[0] == 'A');
    GARRY_CHECK(result[val_size - 1] == (garry_u8)('A' + ((val_size - 1) % 26)));

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value);
    free(result);
}

/**
 * @brief Test storing a value at 4KB.
 * Note: This tests the overflow page mechanism for larger values.
 */
static void test_value_at_4kb(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64];
    garry_u8 *value, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 val_size = 4096;
    garry_i32 i;

    printf("test_value_at_4kb\n");
    cleanup();

    value = (garry_u8 *)malloc(val_size);
    result = (garry_u8 *)malloc(val_size);
    GARRY_CHECK(value != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    key[0] = 'k';
    key[1] = '4';
    key[2] = 'k';
    for (i = 0; i < val_size; i++)
        value[i] = (garry_u8)('A' + (i % 26));

    ok = garry_set(db, txn, key, 3, value, val_size);
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, val_size);
    vlen = 0;
    ok = garry_get(db, txn, key, 3, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == val_size);
    GARRY_CHECK(result[0] == 'A');
    GARRY_CHECK(result[val_size - 1] == (garry_u8)('A' + ((val_size - 1) % 26)));

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value);
    free(result);
}

/**
 * @brief Test storing a value at maximum size (16KB).
 * Note: This tests the overflow page mechanism for maximum-sized values.
 */
static void test_value_at_max_size(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64];
    garry_u8 *value, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 val_size = GARRY_MAX_RECORD_SIZE;
    garry_i32 i;

    printf("test_value_at_max_size\n");
    cleanup();

    value = (garry_u8 *)malloc(val_size);
    result = (garry_u8 *)malloc(val_size);
    GARRY_CHECK(value != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    key[0] = 'm';
    key[1] = 'a';
    key[2] = 'x';
    for (i = 0; i < val_size; i++)
        value[i] = (garry_u8)('A' + (i % 26));

    ok = garry_set(db, txn, key, 3, value, val_size);
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, val_size);
    vlen = 0;
    ok = garry_get(db, txn, key, 3, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == val_size);
    GARRY_CHECK(result[0] == 'A');
    GARRY_CHECK(result[val_size - 1] == (garry_u8)('A' + ((val_size - 1) % 26)));

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value);
    free(result);
}

/**
 * @brief Test multiple large values in the same transaction.
 * Note: Uses values that fit in WAL record inline buffer.
 */
static void test_multiple_large_values(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64];
    garry_u8 *value, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 val_size = 200;
    garry_i32 i, j;

    printf("test_multiple_large_values\n");
    cleanup();

    value = (garry_u8 *)malloc(val_size);
    result = (garry_u8 *)malloc(val_size);
    GARRY_CHECK(value != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    /* Store 10 large values */
    for (j = 0; j < 10; j++)
    {
        memset(key, 0, sizeof(key));
        key[0] = (garry_u8)('a' + j);
        
        for (i = 0; i < val_size; i++)
            value[i] = (garry_u8)('A' + ((i + j) % 26));

        ok = garry_set(db, txn, key, 1, value, val_size);
        GARRY_CHECK(ok == GARRY_OK);
    }

    /* Verify all values */
    for (j = 0; j < 10; j++)
    {
        memset(key, 0, sizeof(key));
        key[0] = (garry_u8)('a' + j);
        
        memset(result, 0, val_size);
        vlen = 0;
        ok = garry_get(db, txn, key, 1, result, &vlen);
        GARRY_CHECK(ok == GARRY_OK);
        GARRY_CHECK(vlen == val_size);
        
        for (i = 0; i < val_size; i++)
        {
            if (result[i] != (garry_u8)('A' + ((i + j) % 26)))
            {
                GARRY_CHECK(0); /* Fail */
            }
        }
    }

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value);
    free(result);
}

/**
 * @brief Test overwriting large values.
 * Note: Uses values that fit in WAL record inline buffer.
 */
static void test_overwrite_large_values(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64];
    garry_u8 *value1, *value2, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 i;

    printf("test_overwrite_large_values\n");
    cleanup();

    value1 = (garry_u8 *)malloc(200);
    value2 = (garry_u8 *)malloc(250);
    result = (garry_u8 *)malloc(250);
    GARRY_CHECK(value1 != NULL);
    GARRY_CHECK(value2 != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    /* Store initial value */
    memset(key, 0, sizeof(key));
    key[0] = 'o';
    key[1] = 'w';
    for (i = 0; i < 200; i++)
        value1[i] = (garry_u8)('A' + (i % 26));
    ok = garry_set(db, txn, key, 2, value1, 200);
    GARRY_CHECK(ok == GARRY_OK);

    /* Overwrite with larger value */
    for (i = 0; i < 250; i++)
        value2[i] = (garry_u8)('B' + (i % 26));
    ok = garry_set(db, txn, key, 2, value2, 250);
    GARRY_CHECK(ok == GARRY_OK);

    /* Verify */
    memset(result, 0, 250);
    vlen = 0;
    ok = garry_get(db, txn, key, 2, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == 250);
    GARRY_CHECK(result[0] == 'B');
    GARRY_CHECK(result[249] == (garry_u8)('B' + (249 % 26)));

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value1);
    free(value2);
    free(result);
}

/**
 * @brief Test storing a value at 9KB.
 */
static void test_value_at_9kb(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64];
    garry_u8 *value, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 val_size = 9000;
    garry_i32 i;

    printf("test_value_at_9kb\n");
    cleanup();

    value = (garry_u8 *)malloc(val_size);
    result = (garry_u8 *)malloc(val_size);
    GARRY_CHECK(value != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    key[0] = '9';
    key[1] = 'k';
    for (i = 0; i < val_size; i++)
        value[i] = (garry_u8)('A' + (i % 26));

    ok = garry_set(db, txn, key, 2, value, val_size);
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, val_size);
    vlen = 0;
    ok = garry_get(db, txn, key, 2, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == val_size);
    GARRY_CHECK(result[0] == 'A');
    GARRY_CHECK(result[val_size - 1] == (garry_u8)('A' + ((val_size - 1) % 26)));

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value);
    free(result);
}

/**
 * @brief Test storing a value at 17KB.
 */
static void test_value_at_17kb(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64];
    garry_u8 *value, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 val_size = 17000;
    garry_i32 i;

    printf("test_value_at_17kb\n");
    cleanup();

    value = (garry_u8 *)malloc(val_size);
    result = (garry_u8 *)malloc(val_size);
    GARRY_CHECK(value != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    key[0] = '1';
    key[1] = '7';
    key[2] = 'k';
    for (i = 0; i < val_size; i++)
        value[i] = (garry_u8)('A' + (i % 26));

    ok = garry_set(db, txn, key, 3, value, val_size);
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, val_size);
    vlen = 0;
    ok = garry_get(db, txn, key, 3, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == val_size);
    GARRY_CHECK(result[0] == 'A');
    GARRY_CHECK(result[val_size - 1] == (garry_u8)('A' + ((val_size - 1) % 26)));

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value);
    free(result);
}

/**
 * @brief Test storing a value at 33KB.
 */
static void test_value_at_33kb(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64];
    garry_u8 *value, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 val_size = 33000;
    garry_i32 i;

    printf("test_value_at_33kb\n");
    cleanup();

    value = (garry_u8 *)malloc(val_size);
    result = (garry_u8 *)malloc(val_size);
    GARRY_CHECK(value != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    key[0] = '3';
    key[1] = '3';
    key[2] = 'k';
    for (i = 0; i < val_size; i++)
        value[i] = (garry_u8)('A' + (i % 26));

    ok = garry_set(db, txn, key, 3, value, val_size);
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, val_size);
    vlen = 0;
    ok = garry_get(db, txn, key, 3, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == val_size);
    GARRY_CHECK(result[0] == 'A');
    GARRY_CHECK(result[val_size - 1] == (garry_u8)('A' + ((val_size - 1) % 26)));

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value);
    free(result);
}

int main(void)
{
    test_value_at_256_bytes();
    test_value_at_257_bytes();
    test_value_at_1kb();
    test_value_at_4kb();
    test_value_at_9kb();
    test_value_at_17kb();
    test_value_at_33kb();
    test_value_at_max_size();
    test_multiple_large_values();
    test_overwrite_large_values();
    if (garry_test_failures == 0)
        printf("test_wal_large_values: ALL PASSED\n");
    return garry_test_failures;
}
