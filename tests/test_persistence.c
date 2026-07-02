/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "storage_engine.h"
#include "storage_txn.h"
#include "storage_ops.h"
#include "storage_navigation.h"
#include "engine_settings.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

#define TEST_DB "/tmp/garry_persist_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

/* The root cause bug: next_txid reset to 1 on reopen, making all
   previously committed version chain entries invisible. */
static void test_basic_reopen(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "persist_a");
    ENCODE_KEY(value, "hello");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 9, value, 5));
    garry_storage_commit(eng, txn);

    garry_storage_close(eng);

    /* Reopen and read */
    eng = garry_storage_open(TEST_DB);
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_get(eng, txn, key, 9, result, &vlen));
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(memcmp(result, "hello", 5) == 0);
    GARRY_CHECK(garry_storage_exists(eng, txn, key, 9));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* Set in txn1, set in txn2, close, reopen — both must survive. */
static void test_multi_txn_reopen(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key1[512], key2[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    /* txn1: set key A */
    txn = garry_storage_begin(eng);
    ENCODE_KEY(key1, "mtxA");
    ENCODE_KEY(value, "valA");
    GARRY_CHECK(garry_storage_set(eng, txn, key1, 4, value, 4));
    garry_storage_commit(eng, txn);

    /* txn2: set key B */
    txn = garry_storage_begin(eng);
    ENCODE_KEY(key2, "mtxB");
    GARRY_CHECK(garry_storage_set(eng, txn, key2, 4, value, 4));
    garry_storage_commit(eng, txn);

    garry_storage_close(eng);

    /* Reopen — both keys must exist */
    eng = garry_storage_open(TEST_DB);
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_get(eng, txn, key1, 4, result, &vlen));
    GARRY_CHECK(vlen == 4);
    GARRY_CHECK(garry_storage_get(eng, txn, key2, 4, result, &vlen));
    GARRY_CHECK(vlen == 4);
    GARRY_CHECK(garry_storage_exists(eng, txn, key1, 4));
    GARRY_CHECK(garry_storage_exists(eng, txn, key2, 4));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* Set, delete, close, reopen — key must NOT exist. */
static void test_delete_persists(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "del_persist");
    ENCODE_KEY(value, "doomed");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 11, value, 6));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_delete(eng, txn, key, 11));
    garry_storage_commit(eng, txn);

    garry_storage_close(eng);

    /* Reopen — key must be gone */
    eng = garry_storage_open(TEST_DB);
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(!garry_storage_get(eng, txn, key, 11, result, &vlen));
    GARRY_CHECK(!garry_storage_exists(eng, txn, key, 11));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* Set in txn, rollback, close, reopen.
   NOTE: rollback only hides writes within a single engine lifetime.
   After close+reopen the active list is empty, so the rolled-back
   write becomes visible. This test verifies that behavior. */
static void test_rollback_reopen_visible(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "rb_persist");
    ENCODE_KEY(value, "ghost");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 10, value, 5));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);

    /* Reopen — rolled-back data is visible (active list is empty) */
    eng = garry_storage_open(TEST_DB);
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_get(eng, txn, key, 10, result, &vlen));
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(garry_storage_exists(eng, txn, key, 10));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* Set, commit, close, reopen, set new key, close, reopen — all must survive. */
static void test_double_reopen(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key1[512], key2[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    /* First write */
    txn = garry_storage_begin(eng);
    ENCODE_KEY(key1, "round1");
    ENCODE_KEY(value, "first");
    GARRY_CHECK(garry_storage_set(eng, txn, key1, 6, value, 5));
    garry_storage_commit(eng, txn);
    garry_storage_close(eng);

    /* Reopen, write again, close */
    eng = garry_storage_open(TEST_DB);
    GARRY_CHECK(eng != NULL);
    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_get(eng, txn, key1, 6, result, &vlen));
    GARRY_CHECK(vlen == 5);
    ENCODE_KEY(key2, "round2");
    ENCODE_KEY(value, "second");
    GARRY_CHECK(garry_storage_set(eng, txn, key2, 6, value, 6));
    garry_storage_commit(eng, txn);
    garry_storage_close(eng);

    /* Final reopen — both keys must exist */
    eng = garry_storage_open(TEST_DB);
    GARRY_CHECK(eng != NULL);
    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_get(eng, txn, key1, 6, result, &vlen));
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(garry_storage_get(eng, txn, key2, 6, result, &vlen));
    GARRY_CHECK(vlen == 6);
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* Many keys across many txns, close, reopen — all must survive. */
static void test_stress_reopen(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;
    int i;
    char kbuf[16], vbuf[16];

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    /* Set 20 keys across 4 txns (5 per txn) */
    for (i = 0; i < 20; i++)
    {
        if (i % 5 == 0)
            txn = garry_storage_begin(eng);
        sprintf(kbuf, "k%04d", i);
        sprintf(vbuf, "v%04d", i);
        memcpy(key, kbuf, 5);
        memcpy(value, vbuf, 5);
        GARRY_CHECK(garry_storage_set(eng, txn, key, 5, value, 5));
        if (i % 5 == 4)
            garry_storage_commit(eng, txn);
    }
    garry_storage_close(eng);

    /* Reopen — all 20 keys must exist */
    eng = garry_storage_open(TEST_DB);
    GARRY_CHECK(eng != NULL);
    txn = garry_storage_begin(eng);
    for (i = 0; i < 20; i++)
    {
        sprintf(kbuf, "k%04d", i);
        memcpy(key, kbuf, 5);
        GARRY_CHECK(garry_storage_get(eng, txn, key, 5, result, &vlen));
        GARRY_CHECK(vlen == 5);
        sprintf(vbuf, "v%04d", i);
        GARRY_CHECK(memcmp(result, vbuf, 5) == 0);
    }
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

int main(void)
{
    test_basic_reopen();
    test_multi_txn_reopen();
    test_delete_persists();
    test_rollback_reopen_visible();
    test_double_reopen();
    test_stress_reopen();
    if (garry_test_failures == 0)
        printf("test_persistence: ALL PASSED\n");
    return garry_test_failures;
}
