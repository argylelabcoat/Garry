/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "lz4.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_large_val_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static void test_value_at_inline_limit(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64];
    garry_u8 *value, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 val_size = 498;
    garry_i32 i;

    printf("test_value_at_inline_limit\n");
    cleanup();

    value = (garry_u8*)malloc(val_size);
    result = (garry_u8*)malloc(val_size);
    GARRY_CHECK(value != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    key[0] = 'n'; key[1] = 'e'; key[2] = 'a'; key[3] = 'r';
    for (i = 0; i < val_size; i++) value[i] = (garry_u8)('A' + (i % 26));

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

static void test_value_just_under_limit(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64];
    garry_u8 *value, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 val_size = 400;
    garry_i32 i;

    printf("test_value_just_under_limit\n");
    cleanup();

    value = (garry_u8*)malloc(val_size);
    result = (garry_u8*)malloc(val_size);
    GARRY_CHECK(value != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    key[0] = 'u'; key[1] = 'n'; key[2] = 'd';
    for (i = 0; i < val_size; i++) value[i] = (garry_u8)(i & 0xFF);

    ok = garry_set(db, txn, key, 3, value, val_size);
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, val_size);
    vlen = 0;
    ok = garry_get(db, txn, key, 3, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == val_size);
    GARRY_CHECK(memcmp(result, value, val_size) == 0);

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value);
    free(result);
}

static void test_overwrite_large_with_small(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64], small_val[8];
    garry_u8 *large_val, *result;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 i;

    printf("test_overwrite_large_with_small\n");
    cleanup();

    large_val = (garry_u8*)malloc(498);
    result = (garry_u8*)malloc(512);
    GARRY_CHECK(large_val != NULL);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    key[0] = 'o'; key[1] = 'w';

    for (i = 0; i < 498; i++) large_val[i] = (garry_u8)('L');
    ok = garry_set(db, txn, key, 2, large_val, 498);
    GARRY_CHECK(ok == GARRY_OK);

    memset(small_val, 'S', sizeof(small_val));
    ok = garry_set(db, txn, key, 2, small_val, sizeof(small_val));
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, 512);
    vlen = 0;
    ok = garry_get(db, txn, key, 2, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == sizeof(small_val));
    GARRY_CHECK(result[0] == 'S');

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(large_val);
    free(result);
}

static void test_delete_large_value(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[64], result[64];
    garry_u8 *large_val;
    garry_i32 vlen;
    garry_status_t ok;
    garry_i32 i;

    printf("test_delete_large_value\n");
    cleanup();

    large_val = (garry_u8*)malloc(498);
    GARRY_CHECK(large_val != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 0, sizeof(key));
    key[0] = 'd'; key[1] = 'l';
    for (i = 0; i < 498; i++) large_val[i] = (garry_u8)('D');

    ok = garry_set(db, txn, key, 2, large_val, 498);
    GARRY_CHECK(ok == GARRY_OK);

    ok = garry_delete(db, txn, key, 2);
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, sizeof(result));
    vlen = 0;
    ok = garry_get(db, txn, key, 2, result, &vlen);
    GARRY_CHECK(ok == GARRY_ERR_NOT_FOUND);

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(large_val);
}

static void test_lz4_round_trip_large(void)
{
    size_t in_len = 1000;
    char *input;
    size_t compressed_len = 0;
    char *compressed;
    size_t decompressed_len = 0;
    char *decompressed;
    size_t i;

    printf("test_lz4_round_trip_large\n");

    input = (char*)malloc(in_len);
    GARRY_CHECK(input != NULL);

    for (i = 0; i < in_len; i++) {
        input[i] = (char)('A' + (i % 26));
    }

    compressed = lz4_compress(input, in_len, &compressed_len);
    GARRY_CHECK(compressed != NULL);
    GARRY_CHECK(compressed_len > 0);

    decompressed = lz4_decompress(compressed, compressed_len, in_len * 2, &decompressed_len);
    GARRY_CHECK(decompressed != NULL);
    GARRY_CHECK(decompressed_len == in_len);
    GARRY_CHECK(memcmp(decompressed, input, in_len) == 0);

    lz4_free(compressed);
    lz4_free(decompressed);
    free(input);
}

int main(void)
{
    test_value_at_inline_limit();
    test_value_just_under_limit();
    test_overwrite_large_with_small();
    test_delete_large_value();
    test_lz4_round_trip_large();
    if (garry_test_failures == 0) printf("test_large_values: ALL PASSED\n");
    return garry_test_failures;
}
