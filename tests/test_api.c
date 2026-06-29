/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

#define TEST_DB "/tmp/garry_full_api.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static void test_database_create_close(void)
{
    garry_database *db;
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    garry_database_close(db);
}

static void test_set_get_single(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[512], value[512], result[512];
    garry_i32 vlen;
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);
    GARRY_CHECK(txn > 0);
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));
    key[0] = 'k'; key[1] = '1';
    value[0] = 'v'; value[1] = '1';
    GARRY_CHECK(garry_set(db, txn, key, 2, value, 2) == GARRY_OK);
    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn, key, 2, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 2);
    GARRY_CHECK(result[0] == 'v');
    GARRY_CHECK(result[1] == '1');
    garry_txn_commit(db, txn);
    garry_database_close(db);
}

static void test_delete(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[512], value[512], result[512];
    garry_i32 vlen;
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));
    key[0] = 'd'; value[0] = '1';
    GARRY_CHECK(garry_set(db, txn, key, 1, value, 1) == GARRY_OK);
    GARRY_CHECK(garry_delete(db, txn, key, 1) == GARRY_OK);
    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn, key, 1, result, &vlen) != GARRY_OK);
    garry_txn_commit(db, txn);
    garry_database_close(db);
}

static void test_get_default(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[512], value[512], result[512], def[512];
    garry_i32 vlen;
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));
    memset(def, 0, sizeof(def));
    key[0] = 'g'; def[0] = 'D';
    GARRY_CHECK(garry_get_default(db, txn, key, 1, def, 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(result[0] == 'D');
    value[0] = 'v';
    garry_set(db, txn, key, 1, value, 1);
    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get_default(db, txn, key, 1, def, 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(result[0] == 'v');
    garry_txn_commit(db, txn);
    garry_database_close(db);
}

static void test_overwrite(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[512], value[512], result[512];
    garry_i32 vlen;
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));
    key[0] = 'o'; value[0] = '1';
    garry_set(db, txn, key, 1, value, 1);
    value[0] = '2';
    garry_set(db, txn, key, 1, value, 1);
    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn, key, 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(result[0] == '2');
    garry_txn_commit(db, txn);
    garry_database_close(db);
}

static void test_cross_txn_read(void)
{
    garry_database *db;
    garry_txn txn1, txn2;
    garry_u8 key[512], value[512], result[512];
    garry_i32 vlen;
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn1 = garry_txn_begin(db);
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));
    key[0] = 'x'; value[0] = '1';
    garry_set(db, txn1, key, 1, value, 1);
    garry_txn_commit(db, txn1);
    txn2 = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn2, key, 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(result[0] == '1');
    garry_txn_rollback(db, txn2);
    garry_database_close(db);
}

static void test_rollback(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[512], value[512];
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));
    key[0] = 'r'; value[0] = '1';
    garry_set(db, txn, key, 1, value, 1);
    garry_txn_rollback(db, txn);
    garry_database_close(db);
}

static void test_multi_key(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[512], value[512], result[512];
    garry_i32 vlen;
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));
    key[0] = 'a'; value[0] = '1';
    GARRY_CHECK(garry_set(db, txn, key, 1, value, 1) == GARRY_OK);
    key[0] = 'b'; value[0] = '2';
    GARRY_CHECK(garry_set(db, txn, key, 1, value, 1) == GARRY_OK);
    key[0] = 'c'; value[0] = '3';
    GARRY_CHECK(garry_set(db, txn, key, 1, value, 1) == GARRY_OK);
    garry_txn_commit(db, txn);
    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result)); vlen = 0;
    GARRY_CHECK(garry_get(db, txn, (const garry_u8*)"a", 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(result[0] == '1');
    memset(result, 0, sizeof(result)); vlen = 0;
    GARRY_CHECK(garry_get(db, txn, (const garry_u8*)"b", 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(result[0] == '2');
    memset(result, 0, sizeof(result)); vlen = 0;
    GARRY_CHECK(garry_get(db, txn, (const garry_u8*)"c", 1, result, &vlen) == GARRY_OK);
    GARRY_CHECK(result[0] == '3');
    garry_txn_rollback(db, txn);
    garry_database_close(db);
}

static void test_bulk(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[512], value[512], result[512];
    garry_i32 vlen;
    garry_i32 i;
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);
    for (i = 0; i < 100; i++) {
        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
        key[0] = 'k';
        key[1] = (garry_u8)('0' + i / 10);
        key[2] = (garry_u8)('0' + i % 10);
        value[0] = (garry_u8)i;
        GARRY_CHECK(garry_set(db, txn, key, 3, value, 1) == GARRY_OK);
    }
    garry_txn_commit(db, txn);
    txn = garry_txn_begin(db);
    for (i = 0; i < 100; i++) {
        memset(key, 0, sizeof(key));
        key[0] = 'k';
        key[1] = (garry_u8)('0' + i / 10);
        key[2] = (garry_u8)('0' + i % 10);
        memset(result, 0, sizeof(result));
        vlen = 0;
        GARRY_CHECK(garry_get(db, txn, key, 3, result, &vlen) == GARRY_OK);
        GARRY_CHECK(vlen == 1);
        GARRY_CHECK(result[0] == (garry_u8)i);
    }
    garry_txn_rollback(db, txn);
    garry_database_close(db);
}

static void test_encoding(void)
{
    garry_byte_array key, result;
    garry_key_tuple t;
    garry_i32 cmp;
    key[0] = 'x';
    garry_empty_byte_array(key);
    GARRY_CHECK(key[0] == 0);
    GARRY_CHECK(key[255] == 0);
    t = garry_make_key_2("accounts", "user1");
    GARRY_CHECK(t.count == 2);
    garry_encode_key_tuple(&t, key);
    GARRY_CHECK(key[0] != 0);
    t = garry_make_key_3("a", "b", "c");
    GARRY_CHECK(t.count == 3);
    garry_encode_key_tuple(&t, key);
    GARRY_CHECK(key[0] != 0);
    t = garry_make_key_4("w", "x", "y", "z");
    GARRY_CHECK(t.count == 4);
    garry_encode_key_tuple(&t, key);
    GARRY_CHECK(key[0] != 0);
    garry_empty_byte_array(result);
    result[0] = 'a'; result[1] = 'b';
    cmp = garry_byte_compare(result, 2, result, 2);
    GARRY_CHECK(cmp == 0);
    cmp = garry_byte_compare((const garry_u8*)"aa", 2, (const garry_u8*)"ab", 2);
    GARRY_CHECK(cmp < 0);
    cmp = garry_byte_compare((const garry_u8*)"ab", 2, (const garry_u8*)"aa", 2);
    GARRY_CHECK(cmp > 0);
    cmp = garry_byte_compare((const garry_u8*)"a", 1, (const garry_u8*)"ab", 2);
    GARRY_CHECK(cmp != 0);
    garry_empty_byte_array(key);
    garry_empty_byte_array(result);
    cmp = garry_byte_compare(key, 0, result, 0);
    GARRY_CHECK(cmp == 0);
}

int main(void)
{
    test_database_create_close();
    test_set_get_single();
    test_delete();
    test_get_default();
    test_overwrite();
    test_cross_txn_read();
    test_rollback();
    test_multi_key();
    test_bulk();
    test_encoding();
    if (garry_test_failures == 0) printf("test_api: ALL PASSED\n");
    return garry_test_failures;
}
