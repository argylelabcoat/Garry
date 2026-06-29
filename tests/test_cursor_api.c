/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "storage_engine.h"
#include "storage_txn.h"
#include "storage_ops.h"
#include "storage_cursor.h"
#include "engine_settings.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

#define TEST_DB "/tmp/garry_cursor_api_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static void test_cursor_iterates_single_key(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512];
    garry_byte ck[512], cv[512];
    garry_i32 klen, vlen;
    garry_storage_cursor cur;
    garry_i32 count = 0;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "cur_a");
    ENCODE_KEY(value, "va");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 5, value, 2));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    cur = garry_storage_cursor_open(eng, txn, NULL, 0);
    while (garry_storage_cursor_next(&cur, ck, &klen, cv, &vlen)) count++;
    garry_storage_cursor_close(&cur);
    GARRY_CHECK(count == 1);
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

static void test_cursor_sees_own_uncommitted(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512];
    garry_byte ck[512], cv[512];
    garry_i32 klen, vlen;
    garry_storage_cursor cur;
    garry_i32 count = 0;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "own_a");
    ENCODE_KEY(value, "va");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 5, value, 2));

    cur = garry_storage_cursor_open(eng, txn, NULL, 0);
    while (garry_storage_cursor_next(&cur, ck, &klen, cv, &vlen)) count++;
    garry_storage_cursor_close(&cur);
    GARRY_CHECK(count == 1);
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    count = 0;
    cur = garry_storage_cursor_open(eng, txn, NULL, 0);
    while (garry_storage_cursor_next(&cur, ck, &klen, cv, &vlen)) count++;
    garry_storage_cursor_close(&cur);
    GARRY_CHECK(count == 1);
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

static void test_cursor_empty_on_fresh_db(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte ck[512], cv[512];
    garry_i32 klen, vlen;
    garry_storage_cursor cur;
    garry_i32 count = 0;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    cur = garry_storage_cursor_open(eng, txn, NULL, 0);
    while (garry_storage_cursor_next(&cur, ck, &klen, cv, &vlen)) count++;
    garry_storage_cursor_close(&cur);
    GARRY_CHECK(count == 0);
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

int main(void)
{
    test_cursor_iterates_single_key();
    test_cursor_sees_own_uncommitted();
    test_cursor_empty_on_fresh_db();
    if (garry_test_failures == 0) printf("test_cursor_api: ALL PASSED\n");
    return garry_test_failures;
}
