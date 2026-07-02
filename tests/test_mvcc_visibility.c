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

#define TEST_DB "/tmp/garry_mvcc_vis_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

/* Bug #1: exists returned 1 after delete because it only checked
   B-tree presence, not MVCC visibility. */
static void test_exists_after_delete(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    /* Set */
    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "del_exists");
    ENCODE_KEY(value, "val");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 11, value, 3));
    garry_storage_commit(eng, txn);

    /* Delete */
    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_delete(eng, txn, key, 11));
    garry_storage_commit(eng, txn);

    /* exists in new txn must return 0 */
    txn = garry_storage_begin(eng);
    GARRY_CHECK(!garry_storage_exists(eng, txn, key, 11));
    GARRY_CHECK(!garry_storage_get(eng, txn, key, 11, result, &vlen));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* Rollback must hide writes from subsequent transactions. */
static void test_rollback_hides_writes(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    /* Set in txn1, rollback */
    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "rb_vis");
    ENCODE_KEY(value, "gone");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 6, value, 4));
    garry_storage_rollback(eng, txn);

    /* get in new txn must return NOT_FOUND */
    txn = garry_storage_begin(eng);
    GARRY_CHECK(!garry_storage_get(eng, txn, key, 6, result, &vlen));

    /* exists in new txn must return 0 */
    GARRY_CHECK(!garry_storage_exists(eng, txn, key, 6));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* Uncommitted writes from an active txn must be invisible to others. */
static void test_active_txn_invisible(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn1, txn2;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    /* Start txn1, set but don't commit */
    txn1 = garry_storage_begin(eng);
    ENCODE_KEY(key, "uncommitted");
    ENCODE_KEY(value, "secret");
    GARRY_CHECK(garry_storage_set(eng, txn1, key, 11, value, 6));

    /* txn2 must not see uncommitted data */
    txn2 = garry_storage_begin(eng);
    GARRY_CHECK(!garry_storage_get(eng, txn2, key, 11, result, &vlen));
    GARRY_CHECK(!garry_storage_exists(eng, txn2, key, 11));
    garry_storage_rollback(eng, txn2);

    garry_storage_rollback(eng, txn1);
    garry_storage_close(eng);
    cleanup();
}

/* Committed data must be visible to new txns. */
static void test_committed_visible(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "visible");
    ENCODE_KEY(value, "yes");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 7, value, 3));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_get(eng, txn, key, 7, result, &vlen));
    GARRY_CHECK(vlen == 3);
    GARRY_CHECK(memcmp(result, "yes", 3) == 0);
    GARRY_CHECK(garry_storage_exists(eng, txn, key, 7));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* Multiple keys: delete one, others must remain. */
static void test_delete_one_of_many(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    /* Set 3 keys */
    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "keep_a");
    ENCODE_KEY(value, "va");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 6, value, 2));
    ENCODE_KEY(key, "ditch_b");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 7, value, 2));
    ENCODE_KEY(key, "keep_c");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 6, value, 2));
    garry_storage_commit(eng, txn);

    /* Delete middle key */
    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "ditch_b");
    GARRY_CHECK(garry_storage_delete(eng, txn, key, 7));
    garry_storage_commit(eng, txn);

    /* Verify: a and c exist, b does not */
    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "keep_a");
    GARRY_CHECK(garry_storage_exists(eng, txn, key, 6));
    ENCODE_KEY(key, "ditch_b");
    GARRY_CHECK(!garry_storage_exists(eng, txn, key, 7));
    ENCODE_KEY(key, "keep_c");
    GARRY_CHECK(garry_storage_exists(eng, txn, key, 6));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* exists for a key that was never set. */
static void test_exists_never_set(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512];

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "ghost");
    GARRY_CHECK(!garry_storage_exists(eng, txn, key, 5));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* Overwrite: set key, then set same key with new value. */
static void test_overwrite_visibility(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 vlen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "ow_key");
    ENCODE_KEY(value, "old");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 6, value, 3));
    garry_storage_commit(eng, txn);

    /* Overwrite */
    txn = garry_storage_begin(eng);
    ENCODE_KEY(value, "new");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 6, value, 3));
    garry_storage_commit(eng, txn);

    /* Read new value */
    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_get(eng, txn, key, 6, result, &vlen));
    GARRY_CHECK(vlen == 3);
    GARRY_CHECK(memcmp(result, "new", 3) == 0);
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

int main(void)
{
    test_exists_after_delete();
    test_rollback_hides_writes();
    test_active_txn_invisible();
    test_committed_visible();
    test_delete_one_of_many();
    test_exists_never_set();
    test_overwrite_visibility();
    if (garry_test_failures == 0)
        printf("test_mvcc_visibility: ALL PASSED\n");
    return garry_test_failures;
}
