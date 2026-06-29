/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "storage_engine.h"
#include "storage_txn.h"
#include "storage_ops.h"
#include "engine_settings.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

#define TEST_DB "/tmp/garry_delete_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static void test_delete_removes_key(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "del1");
    ENCODE_KEY(value, "gone");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 4, value, 4));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_delete(eng, txn, key, 4));
    GARRY_CHECK(!garry_storage_get(eng, txn, key, 4, result, &vlen));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(!garry_storage_get(eng, txn, key, 4, result, &vlen));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

static void test_delete_tombstone_persists(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "tomb");
    ENCODE_KEY(value, "data");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 4, value, 4));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_delete(eng, txn, key, 4));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(!garry_storage_get(eng, txn, key, 4, result, &vlen));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

int main(void)
{
    test_delete_removes_key();
    test_delete_tombstone_persists();
    if (garry_test_failures == 0) printf("test_delete: ALL PASSED\n");
    return garry_test_failures;
}
