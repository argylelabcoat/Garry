/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

#define TEST_DB "/tmp/garry_multi_txn_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

/* 3 txns: txn1 writes A, txn2 writes B, txn3 writes C.
   All committed. All must be visible to a new reader. */
static void test_three_txns_all_visible(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[16], value[16], result[256];
    garry_i32 vlen;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    garry_set(db, txn, (garry_u8 *)"A", 1, (garry_u8 *)"1", 1);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    garry_set(db, txn, (garry_u8 *)"B", 1, (garry_u8 *)"2", 1);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    garry_set(db, txn, (garry_u8 *)"C", 1, (garry_u8 *)"3", 1);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    GARRY_CHECK(garry_get(db, txn, (garry_u8 *)"A", 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 1 && result[0] == '1');
    GARRY_CHECK(garry_get(db, txn, (garry_u8 *)"B", 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 1 && result[0] == '2');
    GARRY_CHECK(garry_get(db, txn, (garry_u8 *)"C", 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 1 && result[0] == '3');
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

/* 3 txns running concurrently: txn1 writes A (uncommitted),
   txn2 writes B (uncommitted), txn3 reads — must see neither. */
static void test_three_uncommitted_invisible(void)
{
    garry_database *db;
    garry_txn txn1, txn2, txn3;
    garry_u8 result[256];
    garry_i32 vlen;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn1 = garry_txn_begin(db);
    garry_set(db, txn1, (garry_u8 *)"A", 1, (garry_u8 *)"1", 1);

    txn2 = garry_txn_begin(db);
    garry_set(db, txn2, (garry_u8 *)"B", 1, (garry_u8 *)"2", 1);

    /* txn3 must not see uncommitted writes from txn1 or txn2 */
    txn3 = garry_txn_begin(db);
    GARRY_CHECK(garry_get(db, txn3, (garry_u8 *)"A", 1, result, &vlen) == GARRY_ERR_NOT_FOUND);
    GARRY_CHECK(garry_get(db, txn3, (garry_u8 *)"B", 1, result, &vlen) == GARRY_ERR_NOT_FOUND);
    garry_txn_commit(db, txn3);

    garry_txn_commit(db, txn1);
    garry_txn_commit(db, txn2);

    garry_database_close(db);
    cleanup();
}

/* 4 txns: txn1 writes A and commits, txn2 writes B (uncommitted),
   txn3 writes C and commits, txn4 reads — must see A and C but not B. */
static void test_mixed_committed_uncommitted(void)
{
    garry_database *db;
    garry_txn txn1, txn2, txn3, txn4;
    garry_u8 result[256];
    garry_i32 vlen;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn1 = garry_txn_begin(db);
    garry_set(db, txn1, (garry_u8 *)"A", 1, (garry_u8 *)"1", 1);
    garry_txn_commit(db, txn1);

    txn2 = garry_txn_begin(db);
    garry_set(db, txn2, (garry_u8 *)"B", 1, (garry_u8 *)"2", 1);

    txn3 = garry_txn_begin(db);
    garry_set(db, txn3, (garry_u8 *)"C", 1, (garry_u8 *)"3", 1);
    garry_txn_commit(db, txn3);

    txn4 = garry_txn_begin(db);
    GARRY_CHECK(garry_get(db, txn4, (garry_u8 *)"A", 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(result[0] == '1');
    GARRY_CHECK(garry_get(db, txn4, (garry_u8 *)"B", 1, result, &vlen) == GARRY_ERR_NOT_FOUND);
    GARRY_CHECK(garry_get(db, txn4, (garry_u8 *)"C", 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(result[0] == '3');
    garry_txn_commit(db, txn4);

    garry_txn_commit(db, txn2);
    garry_database_close(db);
    cleanup();
}

/* Rollback in a 3-txn scenario: txn1 writes A, txn2 writes B and rolls back,
   txn3 reads — must see A but not B. */
static void test_rollback_among_many_txns(void)
{
    garry_database *db;
    garry_txn txn1, txn2, txn3;
    garry_u8 result[256];
    garry_i32 vlen;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn1 = garry_txn_begin(db);
    garry_set(db, txn1, (garry_u8 *)"A", 1, (garry_u8 *)"1", 1);
    garry_txn_commit(db, txn1);

    txn2 = garry_txn_begin(db);
    garry_set(db, txn2, (garry_u8 *)"B", 1, (garry_u8 *)"2", 1);
    garry_txn_rollback(db, txn2);

    txn3 = garry_txn_begin(db);
    GARRY_CHECK(garry_get(db, txn3, (garry_u8 *)"A", 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(result[0] == '1');
    GARRY_CHECK(garry_get(db, txn3, (garry_u8 *)"B", 1, result, &vlen) == GARRY_ERR_NOT_FOUND);
    garry_txn_commit(db, txn3);

    garry_database_close(db);
    cleanup();
}

/* 4 txns writing to the same key — last committer wins. */
static void test_overwrite_across_txns(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 result[256];
    garry_i32 vlen;
    int i;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    for (i = 0; i < 4; i++)
    {
        char vbuf[8];
        sprintf(vbuf, "v%d", i);
        txn = garry_txn_begin(db);
        garry_set(db, txn, (garry_u8 *)"X", 1, (garry_u8 *)vbuf, (garry_i32)strlen(vbuf));
        garry_txn_commit(db, txn);
    }

    txn = garry_txn_begin(db);
    GARRY_CHECK(garry_get(db, txn, (garry_u8 *)"X", 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 2);
    GARRY_CHECK(result[0] == 'v' && result[1] == '3');
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

/* Cursor isolation: txn1 writes keys, txn2 opens cursor before commit. */
static void test_cursor_isolation(void)
{
    garry_database *db;
    garry_txn txn1, txn2;
    garry_u8 key[256], val[256];
    garry_i32 klen, vlen;
    garry_cursor *cur;
    int count;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    /* Pre-populate */
    txn1 = garry_txn_begin(db);
    garry_set(db, txn1, (garry_u8 *)"aaa", 3, (garry_u8 *)"1", 1);
    garry_set(db, txn1, (garry_u8 *)"bbb", 3, (garry_u8 *)"2", 1);
    garry_txn_commit(db, txn1);

    /* txn2 opens cursor, then txn1 adds more keys */
    txn2 = garry_txn_begin(db);
    cur = garry_cursor_open(db, txn2, NULL, 0);
    count = 0;
    while (garry_cursor_next(cur, key, &klen, val, &vlen))
        count++;
    garry_cursor_close(cur);

    /* txn1 adds more keys */
    txn1 = garry_txn_begin(db);
    garry_set(db, txn1, (garry_u8 *)"ccc", 3, (garry_u8 *)"3", 1);
    garry_txn_commit(db, txn1);

    /* txn2's snapshot must still see only 2 keys */
    cur = garry_cursor_open(db, txn2, NULL, 0);
    count = 0;
    while (garry_cursor_next(cur, key, &klen, val, &vlen))
        count++;
    garry_cursor_close(cur);
    GARRY_CHECK(count == 2);

    garry_txn_commit(db, txn2);
    garry_database_close(db);
    cleanup();
}

int main(void)
{
    test_three_txns_all_visible();
    test_three_uncommitted_invisible();
    test_mixed_committed_uncommitted();
    test_rollback_among_many_txns();
    test_overwrite_across_txns();
    test_cursor_isolation();
    if (garry_test_failures == 0)
        printf("test_multi_txn: ALL PASSED\n");
    return garry_test_failures;
}
