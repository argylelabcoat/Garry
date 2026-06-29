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

#define TEST_DB "/tmp/garry_mvcc_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static void test_snapshot_isolation(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn1, txn2;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn1 = garry_storage_begin(eng);
    txn2 = garry_storage_begin(eng);

    ENCODE_KEY(key, "mvcc_a");
    ENCODE_KEY(value, "val_a");
    GARRY_CHECK(garry_storage_set(eng, txn2, key, 6, value, 5));
    garry_storage_commit(eng, txn2);

    GARRY_CHECK(!garry_storage_get(eng, txn1, key, 6, result, &vlen));

    garry_storage_rollback(eng, txn1);

    garry_storage_close(eng);
    cleanup();
}

static void test_new_txn_sees_committed(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn1, txn2, txn3;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn1 = garry_storage_begin(eng);
    txn2 = garry_storage_begin(eng);

    ENCODE_KEY(key, "mvcc_b");
    ENCODE_KEY(value, "val_b");
    GARRY_CHECK(garry_storage_set(eng, txn2, key, 6, value, 5));
    garry_storage_commit(eng, txn2);

    txn3 = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_get(eng, txn3, key, 6, result, &vlen));
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(memcmp(result, "val_b", 5) == 0);
    garry_storage_rollback(eng, txn3);

    garry_storage_rollback(eng, txn1);
    garry_storage_close(eng);
    cleanup();
}

static void test_separate_txn_writes_visible(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "mvcc_c");
    ENCODE_KEY(value, "val_c");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 6, value, 5));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_get(eng, txn, key, 6, result, &vlen));
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(memcmp(result, "val_c", 5) == 0);
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

int main(void)
{
    test_snapshot_isolation();
    test_new_txn_sees_committed();
    test_separate_txn_writes_visible();
    if (garry_test_failures == 0) printf("test_mvcc: ALL PASSED\n");
    return garry_test_failures;
}
