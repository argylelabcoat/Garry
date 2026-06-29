/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

#define TEST_DB "/tmp/garry_pub_full_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static void test_database_open_close_reopen(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 result[512];
    garry_i32 vlen;
    garry_status_t ok;

    printf("test_database_open_close_reopen\n");
    cleanup();

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    ok = garry_set_str(db, txn, "key1", "val1");
    GARRY_CHECK(ok == GARRY_OK);
    garry_txn_commit(db, txn);
    garry_database_close(db);

    db = garry_database_open(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    vlen = 0;
    ok = garry_get(db, txn, (const garry_u8 *)"key1", 4, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == 4);
    GARRY_CHECK(memcmp(result, "val1", 4) == 0);
    garry_txn_rollback(db, txn);

    garry_database_close(db);
}

static void test_cursor_next_key(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[512];
    garry_i32 klen;
    garry_cursor *cur;
    garry_i32 count = 0;
    garry_status_t ok;

    printf("test_cursor_next_key\n");
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    ok = garry_set_str(db, txn, "aaa", "1");
    GARRY_CHECK(ok == GARRY_OK);
    ok = garry_set_str(db, txn, "bbb", "2");
    GARRY_CHECK(ok == GARRY_OK);
    ok = garry_set_str(db, txn, "ccc", "3");
    GARRY_CHECK(ok == GARRY_OK);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    cur = garry_cursor_open(db, txn, NULL, 0);
    GARRY_CHECK(cur != NULL);

    while (garry_cursor_next_key(cur, key, &klen))
    {
        count++;
        GARRY_CHECK(klen == 3);
    }
    garry_cursor_close(cur);
    GARRY_CHECK(count == 3);
    garry_txn_rollback(db, txn);

    garry_database_close(db);
}

static void test_first_and_last_key(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[512];
    garry_i32 klen;
    garry_bool found;
    garry_status_t ok;

    printf("test_first_and_last_key\n");
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    ok = garry_set_str(db, txn, "bbb", "2");
    GARRY_CHECK(ok == GARRY_OK);
    ok = garry_set_str(db, txn, "aaa", "1");
    GARRY_CHECK(ok == GARRY_OK);
    ok = garry_set_str(db, txn, "ccc", "3");
    GARRY_CHECK(ok == GARRY_OK);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    memset(key, 0, sizeof(key));
    klen = 0;
    found = garry_first(db, txn, key, &klen);
    GARRY_CHECK(found == 1);
    GARRY_CHECK(klen == 3);
    GARRY_CHECK(memcmp(key, "aaa", 3) == 0);

    memset(key, 0, sizeof(key));
    klen = 0;
    found = garry_last(db, txn, key, &klen);
    GARRY_CHECK(found == 1);
    GARRY_CHECK(klen == 3);
    GARRY_CHECK(memcmp(key, "ccc", 3) == 0);
    garry_txn_rollback(db, txn);

    garry_database_close(db);
}

static void test_next_prev_key_navigation(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[512];
    garry_i32 klen;
    garry_bool found;
    garry_status_t ok;

    printf("test_next_prev_key_navigation\n");
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    ok = garry_set_str(db, txn, "aaa", "1");
    GARRY_CHECK(ok == GARRY_OK);
    ok = garry_set_str(db, txn, "bbb", "2");
    GARRY_CHECK(ok == GARRY_OK);
    ok = garry_set_str(db, txn, "ccc", "3");
    GARRY_CHECK(ok == GARRY_OK);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);

    found = garry_next_key(db, txn, (const garry_u8 *)"aaa", 3, key, &klen);
    GARRY_CHECK(found == 1);
    GARRY_CHECK(klen == 3);
    GARRY_CHECK(memcmp(key, "bbb", 3) == 0);

    found = garry_next_key(db, txn, (const garry_u8 *)"bbb", 3, key, &klen);
    GARRY_CHECK(found == 1);
    GARRY_CHECK(klen == 3);
    GARRY_CHECK(memcmp(key, "ccc", 3) == 0);

    found = garry_next_key(db, txn, (const garry_u8 *)"ccc", 3, key, &klen);
    GARRY_CHECK(found == 0);

    found = garry_prev_key(db, txn, (const garry_u8 *)"ccc", 3, key, &klen);
    GARRY_CHECK(found == 1);
    GARRY_CHECK(klen == 3);
    GARRY_CHECK(memcmp(key, "bbb", 3) == 0);

    found = garry_prev_key(db, txn, (const garry_u8 *)"bbb", 3, key, &klen);
    GARRY_CHECK(found == 1);
    GARRY_CHECK(klen == 3);
    GARRY_CHECK(memcmp(key, "aaa", 3) == 0);

    found = garry_prev_key(db, txn, (const garry_u8 *)"aaa", 3, key, &klen);
    GARRY_CHECK(found == 0);

    garry_txn_rollback(db, txn);
    garry_database_close(db);
}

static void test_count_keys(void)
{
    garry_database *db;
    garry_txn txn;
    garry_i32 cnt;
    garry_status_t ok;

    printf("test_count_keys\n");
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    cnt = garry_count(db, txn);
    GARRY_CHECK(cnt == 0);

    ok = garry_set_str(db, txn, "a", "1");
    GARRY_CHECK(ok == GARRY_OK);
    ok = garry_set_str(db, txn, "b", "2");
    GARRY_CHECK(ok == GARRY_OK);
    ok = garry_set_str(db, txn, "c", "3");
    GARRY_CHECK(ok == GARRY_OK);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    cnt = garry_count(db, txn);
    GARRY_CHECK(cnt == 3);
    garry_txn_rollback(db, txn);

    garry_database_close(db);
}

static void test_get_str(void)
{
    garry_database *db;
    garry_txn txn;
    char result[256];
    garry_status_t ok;

    printf("test_get_str\n");
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    ok = garry_set_str(db, txn, "greeting", "hello world");
    GARRY_CHECK(ok == GARRY_OK);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    ok = garry_get_str(db, txn, "greeting", result, sizeof(result));
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(strcmp(result, "hello world") == 0);

    ok = garry_get_str(db, txn, "nonexistent", result, sizeof(result));
    GARRY_CHECK(ok == GARRY_ERR_NOT_FOUND);

    garry_txn_rollback(db, txn);
    garry_database_close(db);
}

static int for_each_counter;
static void count_visitor(const garry_u8 *key, garry_i32 klen, const garry_u8 *val, garry_i32 vlen,
                          void *user_data)
{
    (void)key;
    (void)klen;
    (void)val;
    (void)vlen;
    (void)user_data;
    for_each_counter++;
}

static void test_for_each_iteration(void)
{
    garry_database *db;
    garry_txn txn;
    garry_status_t ok;

    printf("test_for_each_iteration\n");
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    ok = garry_set_str(db, txn, "item/1", "a");
    GARRY_CHECK(ok == GARRY_OK);
    ok = garry_set_str(db, txn, "item/2", "b");
    GARRY_CHECK(ok == GARRY_OK);
    ok = garry_set_str(db, txn, "item/3", "c");
    GARRY_CHECK(ok == GARRY_OK);
    ok = garry_set_str(db, txn, "other/1", "x");
    GARRY_CHECK(ok == GARRY_OK);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);

    for_each_counter = 0;
    garry_for_each(db, txn, NULL, 0, count_visitor, NULL);
    GARRY_CHECK(for_each_counter == 4);

    for_each_counter = 0;
    garry_for_each(db, txn, (const garry_u8 *)"item/", 5, count_visitor, NULL);
    GARRY_CHECK(for_each_counter == 3);

    garry_txn_rollback(db, txn);
    garry_database_close(db);
}

int main(void)
{
    test_database_open_close_reopen();
    test_cursor_next_key();
    test_first_and_last_key();
    test_next_prev_key_navigation();
    test_count_keys();
    test_get_str();
    test_for_each_iteration();
    if (garry_test_failures == 0)
        printf("test_public_api_full: ALL PASSED\n");
    return garry_test_failures;
}
