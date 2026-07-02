/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

#define TEST_DB "/tmp/garry_btree_merge_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

/* Insert enough keys to split, then delete half — must still work. */
static void test_delete_after_split(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[16], value[16], result[256];
    garry_i32 vlen;
    int i;
    char kbuf[16];

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    /* Insert 8 keys to force multiple leaf splits (max 2 per leaf) */
    txn = garry_txn_begin(db);
    for (i = 0; i < 8; i++)
    {
        sprintf(kbuf, "k%02d", i);
        memcpy(key, kbuf, 3);
        garry_set(db, txn, key, 3, (garry_u8 *)"v", 1);
    }
    garry_txn_commit(db, txn);

    /* Delete first 4 keys */
    txn = garry_txn_begin(db);
    for (i = 0; i < 4; i++)
    {
        sprintf(kbuf, "k%02d", i);
        memcpy(key, kbuf, 3);
        garry_delete(db, txn, key, 3);
    }
    garry_txn_commit(db, txn);

    /* Remaining 4 keys must still be accessible */
    txn = garry_txn_begin(db);
    for (i = 4; i < 8; i++)
    {
        sprintf(kbuf, "k%02d", i);
        memcpy(key, kbuf, 3);
        GARRY_CHECK(garry_get(db, txn, key, 3, result, &vlen) == GARRY_OK);
    }
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

/* Delete all keys from a split tree — first/last must return false. */
static void test_delete_all_from_split_tree(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[16], value[16], result[256];
    garry_i32 klen;
    int i;
    char kbuf[16];

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    /* Insert 6 keys */
    txn = garry_txn_begin(db);
    for (i = 0; i < 6; i++)
    {
        sprintf(kbuf, "del%02d", i);
        memcpy(key, kbuf, 5);
        garry_set(db, txn, key, 5, (garry_u8 *)"x", 1);
    }
    garry_txn_commit(db, txn);

    /* Delete all */
    txn = garry_txn_begin(db);
    for (i = 0; i < 6; i++)
    {
        sprintf(kbuf, "del%02d", i);
        memcpy(key, kbuf, 5);
        garry_delete(db, txn, key, 5);
    }
    garry_txn_commit(db, txn);

    /* first/last must return false */
    txn = garry_txn_begin(db);
    GARRY_CHECK(!garry_first(db, txn, result, &klen));
    GARRY_CHECK(!garry_last(db, txn, result, &klen));
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

/* Interleaved insert and delete across multiple txns. */
static void test_interleaved_insert_delete(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[16], value[16], result[256];
    garry_i32 vlen;
    int i;
    char kbuf[16];

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    /* txn1: insert keys 0-5 */
    txn = garry_txn_begin(db);
    for (i = 0; i < 6; i++)
    {
        sprintf(kbuf, "ik%02d", i);
        memcpy(key, kbuf, 4);
        garry_set(db, txn, key, 4, (garry_u8 *)"a", 1);
    }
    garry_txn_commit(db, txn);

    /* txn2: delete keys 0-2, insert keys 6-8 */
    txn = garry_txn_begin(db);
    for (i = 0; i < 3; i++)
    {
        sprintf(kbuf, "ik%02d", i);
        memcpy(key, kbuf, 4);
        garry_delete(db, txn, key, 4);
    }
    for (i = 6; i < 9; i++)
    {
        sprintf(kbuf, "ik%02d", i);
        memcpy(key, kbuf, 4);
        garry_set(db, txn, key, 4, (garry_u8 *)"b", 1);
    }
    garry_txn_commit(db, txn);

    /* Verify state: keys 3-8 exist, 0-2 don't */
    txn = garry_txn_begin(db);
    for (i = 0; i < 9; i++)
    {
        sprintf(kbuf, "ik%02d", i);
        memcpy(key, kbuf, 4);
        if (i < 3)
            GARRY_CHECK(!garry_exists(db, txn, key, 4));
        else
            GARRY_CHECK(garry_exists(db, txn, key, 4));
    }
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

/* Large batch delete: insert 20, delete 15, verify remaining 5. */
static void test_large_batch_delete(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[16], value[16], result[256];
    garry_i32 vlen;
    int i;
    char kbuf[16];

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    for (i = 0; i < 20; i++)
    {
        sprintf(kbuf, "bk%02d", i);
        memcpy(key, kbuf, 4);
        garry_set(db, txn, key, 4, (garry_u8 *)"v", 1);
    }
    garry_txn_commit(db, txn);

    /* Delete 0-14 */
    txn = garry_txn_begin(db);
    for (i = 0; i < 15; i++)
    {
        sprintf(kbuf, "bk%02d", i);
        memcpy(key, kbuf, 4);
        garry_delete(db, txn, key, 4);
    }
    garry_txn_commit(db, txn);

    /* Keys 15-19 must exist */
    txn = garry_txn_begin(db);
    for (i = 15; i < 20; i++)
    {
        sprintf(kbuf, "bk%02d", i);
        memcpy(key, kbuf, 4);
        GARRY_CHECK(garry_get(db, txn, key, 4, result, &vlen) == GARRY_OK);
    }
    /* Keys 0-14 must not */
    for (i = 0; i < 15; i++)
    {
        sprintf(kbuf, "bk%02d", i);
        memcpy(key, kbuf, 4);
        GARRY_CHECK(garry_get(db, txn, key, 4, result, &vlen) == GARRY_ERR_NOT_FOUND);
    }
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

/* Delete and re-insert the same key in the same txn. */
static void test_delete_reinsert(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 result[256];
    garry_i32 vlen;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    garry_set(db, txn, (garry_u8 *)"dr", 2, (garry_u8 *)"first", 5);
    garry_delete(db, txn, (garry_u8 *)"dr", 2);
    garry_set(db, txn, (garry_u8 *)"dr", 2, (garry_u8 *)"second", 6);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    GARRY_CHECK(garry_get(db, txn, (garry_u8 *)"dr", 2, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 6);
    GARRY_CHECK(memcmp(result, "second", 6) == 0);
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

int main(void)
{
    test_delete_after_split();
    test_delete_all_from_split_tree();
    test_interleaved_insert_delete();
    test_large_batch_delete();
    test_delete_reinsert();
    if (garry_test_failures == 0)
        printf("test_btree_merge: ALL PASSED\n");
    return garry_test_failures;
}
