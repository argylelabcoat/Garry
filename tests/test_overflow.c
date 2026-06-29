/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

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

static void test_set_klen_too_large(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[300], value[5];
    garry_status_t ok;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 'A', sizeof(key));
    memset(value, 'V', sizeof(value));

    ok = garry_set(db, txn, key, 257, value, 5);
    GARRY_CHECK(ok != GARRY_OK);

    garry_txn_commit(db, txn);
    garry_database_close(db);
}

static void test_set_klen_at_boundary(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[256], value[5], result[256];
    garry_i32 vlen;
    garry_status_t ok;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 'B', sizeof(key));
    memset(value, 'V', sizeof(value));
    memset(result, 0, sizeof(result));

    /* CBOR byte-string encoding handles lengths up to 255 in 1-byte header.
       klen=255 is the maximum that round-trips correctly. */
    ok = garry_set(db, txn, key, 255, value, 5);
    GARRY_CHECK(ok == GARRY_OK);

    vlen = 0;
    ok = garry_get(db, txn, key, 255, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(result[0] == 'V');

    garry_txn_commit(db, txn);
    garry_database_close(db);
}

static void test_set_klen_zero(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 value[5];
    garry_status_t ok;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(value, 'V', sizeof(value));

    ok = garry_set(db, txn, (const garry_u8 *)"", 0, value, 5);
    GARRY_CHECK(ok != GARRY_OK);

    garry_txn_commit(db, txn);
    garry_database_close(db);
}

static void test_get_klen_too_large(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[300], result[5];
    garry_i32 vlen;
    garry_status_t ok;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 'C', sizeof(key));
    memset(result, 0, sizeof(result));

    vlen = sizeof(result);
    ok = garry_get(db, txn, key, 257, result, &vlen);
    GARRY_CHECK(ok != GARRY_OK);

    garry_txn_commit(db, txn);
    garry_database_close(db);
}

static void test_set_str_key_too_long(void)
{
    garry_database *db;
    garry_txn txn;
    char key[301];
    garry_status_t ok;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    memset(key, 'D', sizeof(key) - 1);
    key[sizeof(key) - 1] = '\0';

    ok = garry_set_str(db, txn, key, "value");
    GARRY_CHECK(ok != GARRY_OK);

    garry_txn_commit(db, txn);
    garry_database_close(db);
}

int main(void)
{
    test_set_klen_too_large();
    test_set_klen_at_boundary();
    test_set_klen_zero();
    test_get_klen_too_large();
    test_set_str_key_too_long();
    if (garry_test_failures == 0)
        printf("test_overflow: ALL PASSED\n");
    return garry_test_failures;
}
