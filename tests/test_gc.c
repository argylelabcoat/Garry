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
#include "gc.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

#define TEST_DB "/tmp/garry_gc_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

/* GC on a database with only committed data — no pruning needed. */
static void test_gc_no_active_txns(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "gc_a");
    ENCODE_KEY(value, "val_a");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 4, value, 5));
    garry_storage_commit(eng, txn);

    /* GC should not crash or lose data */
    garry_mvcc_gc(eng);

    /* Data must still be readable */
    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_get(eng, txn, key, 4, result, &vlen));
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(memcmp(result, "val_a", 5) == 0);
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* GC while a long-lived txn holds old versions visible. */
static void test_gc_preserves_visible(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn1, txn2;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    /* First write */
    txn2 = garry_storage_begin(eng);
    ENCODE_KEY(key, "gc_b");
    ENCODE_KEY(value, "new");
    GARRY_CHECK(garry_storage_set(eng, txn2, key, 4, value, 3));
    garry_storage_commit(eng, txn2);

    /* txn1 starts AFTER first write — must see "new" */
    txn1 = garry_storage_begin(eng);

    /* Overwrite */
    txn2 = garry_storage_begin(eng);
    ENCODE_KEY(value, "newer");
    GARRY_CHECK(garry_storage_set(eng, txn2, key, 4, value, 5));
    garry_storage_commit(eng, txn2);

    /* GC should not prune versions visible to txn1 */
    garry_mvcc_gc(eng);

    /* txn1 must still see the value it saw when it started */
    GARRY_CHECK(garry_storage_get(eng, txn1, key, 4, result, &vlen));
    GARRY_CHECK(vlen == 3);
    GARRY_CHECK(memcmp(result, "new", 3) == 0);
    garry_storage_rollback(eng, txn1);

    /* New txn sees latest */
    txn2 = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_get(eng, txn2, key, 4, result, &vlen));
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(memcmp(result, "newer", 5) == 0);
    garry_storage_rollback(eng, txn2);

    garry_storage_close(eng);
    cleanup();
}

/* GC after all txns close — old versions can be pruned. */
static void test_gc_prunes_old_versions(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;
    int i;
    char vbuf[16];

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    /* Write and overwrite 10 times */
    for (i = 0; i < 10; i++)
    {
        txn = garry_storage_begin(eng);
        ENCODE_KEY(key, "gc_c");
        sprintf(vbuf, "ver%02d", i);
        memcpy(value, vbuf, 5);
        GARRY_CHECK(garry_storage_set(eng, txn, key, 4, value, 5));
        garry_storage_commit(eng, txn);
    }

    /* GC should prune old versions (no active txns) */
    garry_mvcc_gc(eng);

    /* Latest value must survive */
    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_get(eng, txn, key, 4, result, &vlen));
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(memcmp(result, "ver09", 5) == 0);
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* GC on empty database — must not crash. */
static void test_gc_empty_database(void)
{
    garry_engine_handle *eng;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    garry_mvcc_gc(eng);

    garry_storage_close(eng);
    cleanup();
}

/* GC with deleted keys — tombstones should remain visible to active txns. */
static void test_gc_with_deletes(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "gc_d");
    ENCODE_KEY(value, "here");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 4, value, 4));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_delete(eng, txn, key, 4));
    garry_storage_commit(eng, txn);

    garry_mvcc_gc(eng);

    /* Key must still be gone */
    txn = garry_storage_begin(eng);
    GARRY_CHECK(!garry_storage_get(eng, txn, key, 4, result, &vlen));
    GARRY_CHECK(!garry_storage_exists(eng, txn, key, 4));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

int main(void)
{
    test_gc_no_active_txns();
    test_gc_preserves_visible();
    test_gc_prunes_old_versions();
    test_gc_empty_database();
    test_gc_with_deletes();
    if (garry_test_failures == 0)
        printf("test_gc: ALL PASSED\n");
    return garry_test_failures;
}
