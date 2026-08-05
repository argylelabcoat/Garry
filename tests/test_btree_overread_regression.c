/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * Regression coverage for a fixed bug: garry_leaf_insert (btree_node.c) and
 * btree_insert_rec's update-in-place and split-and-insert paths
 * (btree_modify.c) used to memcpy sizeof(garry_byte_array) (256) bytes from
 * the caller-supplied key/value pointers instead of the real klen/vlen,
 * over-reading past the end of any source buffer shorter than 256 bytes.
 *
 * Every value here is heap-allocated with malloc() at its exact real size
 * (not a stack array or a pre-sized garry_byte_array), so an ASan build
 * deterministically catches a heap-buffer-overflow read on any regression,
 * without depending on thread/stack layout luck the way the original crash
 * report did.
 */
#include "garry/api.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_overread_regression_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal.ckpt");
}

static garry_u8 *make_exact_value(garry_i32 len, garry_u8 fill)
{
    garry_u8 *v = (garry_u8 *)malloc((size_t)len);
    garry_i32 i;
    GARRY_CHECK(v != NULL);
    for (i = 0; i < len; i++)
        v[i] = (garry_u8)(fill + (i % 7));
    return v;
}

/* New-key insert into an empty leaf: exercises garry_leaf_insert's
 * idx == entry_count path (btree_node.c). */
static void test_new_key_insert_small_value(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[8] = "small1";
    garry_u8 *value, *result;
    garry_i32 vlen;
    garry_status_t ok;

    printf("test_new_key_insert_small_value\n");
    cleanup();

    value = make_exact_value(5, 'A');
    result = (garry_u8 *)malloc(5);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    ok = garry_set(db, txn, key, 6, value, 5);
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, 5);
    vlen = 0;
    ok = garry_get(db, txn, key, 6, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(memcmp(result, value, 5) == 0);

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value);
    free(result);
}

/* Update an existing key: exercises btree_insert_rec's idx >= 0
 * ("found" / overwrite in place) path in btree_modify.c. */
static void test_update_existing_key_small_value(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[8] = "upd1";
    garry_u8 *value1, *value2, *result;
    garry_i32 vlen;
    garry_status_t ok;

    printf("test_update_existing_key_small_value\n");
    cleanup();

    value1 = make_exact_value(6, 'B');
    value2 = make_exact_value(4, 'C');
    result = (garry_u8 *)malloc(6);
    GARRY_CHECK(result != NULL);

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    ok = garry_set(db, txn, key, 4, value1, 6);
    GARRY_CHECK(ok == GARRY_OK);
    ok = garry_set(db, txn, key, 4, value2, 4);
    GARRY_CHECK(ok == GARRY_OK);

    memset(result, 0, 6);
    vlen = 0;
    ok = garry_get(db, txn, key, 4, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == 4);
    GARRY_CHECK(memcmp(result, value2, 4) == 0);

    garry_txn_commit(db, txn);
    garry_database_close(db);
    free(value1);
    free(value2);
    free(result);
}

/* GARRY_MAX_KEYS_PER_NODE is 3, so a 4th distinct key forces a leaf split,
 * exercising btree_modify.c's split-and-insert path (the third fixed call
 * site) with a small, exact-size heap value. */
static void test_split_forces_small_value_copy(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 keys[5][8] = {"splitA", "splitB", "splitC", "splitD", "splitE"};
    garry_u8 *values[5];
    garry_u8 result[16];
    garry_i32 vlen;
    garry_status_t ok;
    int i;

    printf("test_split_forces_small_value_copy\n");
    cleanup();

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);

    for (i = 0; i < 5; i++)
    {
        values[i] = make_exact_value(3 + i, (garry_u8)('P' + i));
        ok = garry_set(db, txn, keys[i], 6, values[i], 3 + i);
        GARRY_CHECK(ok == GARRY_OK);
    }

    for (i = 0; i < 5; i++)
    {
        memset(result, 0, sizeof(result));
        vlen = 0;
        ok = garry_get(db, txn, keys[i], 6, result, &vlen);
        GARRY_CHECK(ok == GARRY_OK);
        GARRY_CHECK(vlen == 3 + i);
        GARRY_CHECK(memcmp(result, values[i], (size_t)(3 + i)) == 0);
        free(values[i]);
    }

    garry_txn_commit(db, txn);
    garry_database_close(db);
}

int main(void)
{
    test_new_key_insert_small_value();
    test_update_existing_key_small_value();
    test_split_forces_small_value_copy();
    cleanup();
    if (garry_test_failures == 0)
        printf("test_btree_overread_regression: ALL PASSED\n");
    return garry_test_failures;
}
