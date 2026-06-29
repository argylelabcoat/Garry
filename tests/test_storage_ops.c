/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "storage_ops.h"
#include "storage_engine.h"
#include "storage_txn.h"
#include "engine_settings.h"
#include "test_helpers.h"
#include <string.h>
#include <stdio.h>

#define TEST_DB "/tmp/garry_storage_ops_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static void test_set_and_get_single_key(void)
{
    garry_engine_handle *eng;
    garry_engine_settings settings;
    garry_txn_id txn;
    garry_byte key[512];
    garry_byte result[512];
    garry_i32 result_len;

    cleanup();
    settings = garry_default_engine_settings();
    eng = garry_storage_init(TEST_DB, settings);
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(txn > 0);

    ENCODE_KEY(key, "hello");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 5, (const garry_byte*)"world", 5));

    result_len = 0;
    memset(result, 0, sizeof(result));
    GARRY_CHECK(garry_storage_get(eng, txn, key, 5, result, &result_len));
    GARRY_CHECK(result_len == 5);
    GARRY_CHECK(memcmp(result, "world", 5) == 0);

    garry_storage_commit(eng, txn);
    garry_storage_close(eng);
    cleanup();
}

static void test_set_overwrites_same_key(void)
{
    garry_engine_handle *eng;
    garry_engine_settings settings;
    garry_txn_id txn;
    garry_byte key[512];
    garry_byte result[512];
    garry_i32 result_len;

    cleanup();
    settings = garry_default_engine_settings();
    eng = garry_storage_init(TEST_DB, settings);
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "key1");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 4, (const garry_byte*)"val1", 4));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_set(eng, txn, key, 4, (const garry_byte*)"val2", 4));

    result_len = 0;
    memset(result, 0, sizeof(result));
    GARRY_CHECK(garry_storage_get(eng, txn, key, 4, result, &result_len));
    GARRY_CHECK(result_len == 4);
    GARRY_CHECK(memcmp(result, "val2", 4) == 0);

    garry_storage_commit(eng, txn);
    garry_storage_close(eng);
    cleanup();
}

static void test_get_nonexistent_key_returns_false(void)
{
    garry_engine_handle *eng;
    garry_engine_settings settings;
    garry_txn_id txn;
    garry_byte key[512];
    garry_byte result[512];
    garry_i32 result_len;

    cleanup();
    settings = garry_default_engine_settings();
    eng = garry_storage_init(TEST_DB, settings);
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "nope");
    result_len = 0;
    GARRY_CHECK(!garry_storage_get(eng, txn, key, 4, result, &result_len));

    garry_storage_commit(eng, txn);
    garry_storage_close(eng);
    cleanup();
}

static void test_delete_key(void)
{
    garry_engine_handle *eng;
    garry_engine_settings settings;
    garry_txn_id txn;
    garry_byte key[512];

    cleanup();
    settings = garry_default_engine_settings();
    eng = garry_storage_init(TEST_DB, settings);
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "delme");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 5, (const garry_byte*)"gone", 4));
    GARRY_CHECK(garry_storage_delete(eng, txn, key, 5));
    garry_storage_commit(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

static void test_get_after_delete_returns_false(void)
{
    garry_engine_handle *eng;
    garry_engine_settings settings;
    garry_txn_id txn;
    garry_byte key[512];
    garry_byte result[512];
    garry_i32 result_len;

    cleanup();
    settings = garry_default_engine_settings();
    eng = garry_storage_init(TEST_DB, settings);
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "deldel");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 6, (const garry_byte*)"temp", 4));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_delete(eng, txn, key, 6));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    result_len = 0;
    GARRY_CHECK(!garry_storage_get(eng, txn, key, 6, result, &result_len));

    garry_storage_commit(eng, txn);
    garry_storage_close(eng);
    cleanup();
}

static void test_get_default_returns_value_when_exists(void)
{
    garry_engine_handle *eng;
    garry_engine_settings settings;
    garry_txn_id txn;
    garry_byte key[512];
    garry_byte result[512];
    garry_i32 result_len;

    cleanup();
    settings = garry_default_engine_settings();
    eng = garry_storage_init(TEST_DB, settings);
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "exists");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 6, (const garry_byte*)"real", 4));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    result_len = 0;
    GARRY_CHECK(garry_storage_get_default(eng, txn, key, 6,
                 (const garry_byte*)"def", 3, result, &result_len));
    GARRY_CHECK(result_len == 4);
    GARRY_CHECK(memcmp(result, "real", 4) == 0);

    garry_storage_commit(eng, txn);
    garry_storage_close(eng);
    cleanup();
}

static void test_get_default_returns_default_when_missing(void)
{
    garry_engine_handle *eng;
    garry_engine_settings settings;
    garry_txn_id txn;
    garry_byte key[512];
    garry_byte result[512];
    garry_i32 result_len;

    cleanup();
    settings = garry_default_engine_settings();
    eng = garry_storage_init(TEST_DB, settings);
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "missing");
    result_len = 0;
    GARRY_CHECK(garry_storage_get_default(eng, txn, key, 7,
                 (const garry_byte*)"fallback", 8, result, &result_len));
    GARRY_CHECK(result_len == 8);
    GARRY_CHECK(memcmp(result, "fallback", 8) == 0);

    garry_storage_commit(eng, txn);
    garry_storage_close(eng);
    cleanup();
}

static void test_storage_data_returns_correct_flags(void)
{
    garry_engine_handle *eng;
    garry_engine_settings settings;
    garry_txn_id txn;
    garry_byte key[512];

    cleanup();
    settings = garry_default_engine_settings();
    eng = garry_storage_init(TEST_DB, settings);
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "nodata");
    GARRY_CHECK(garry_storage_data(eng, txn, key, 6) == 0);

    ENCODE_KEY(key, "hasval");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 6, (const garry_byte*)"x", 1));
    GARRY_CHECK(garry_storage_data(eng, txn, key, 6) == GARRY_DATA_HAS_VALUE);

    garry_storage_commit(eng, txn);
    garry_storage_close(eng);
    cleanup();
}

int main(void)
{
    test_set_and_get_single_key();
    test_set_overwrites_same_key();
    test_get_nonexistent_key_returns_false();
    test_delete_key();
    test_get_after_delete_returns_false();
    test_get_default_returns_value_when_exists();
    test_get_default_returns_default_when_missing();
    test_storage_data_returns_correct_flags();

    if (garry_test_failures == 0) printf("test_storage_ops: ALL PASSED\n");
    return garry_test_failures;
}
