/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

#define TEST_DB "/tmp/garry_wal_recovery_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal.ckpt");
}

/* 1. Set 3 keys, close, reopen, get all 3 */
static void test_basic_recovery(void)
{
    garry_database *db;
    garry_txn txn;
    garry_byte_array key1, key2, key3, val1, val2, val3, result;
    garry_i32 vlen;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    ENCODE_KEY(key1, "alpha");
    ENCODE_KEY(val1, "one");
    GARRY_CHECK(garry_set(db, txn, key1, 5, val1, 3) == GARRY_OK);
    ENCODE_KEY(key2, "bravo");
    ENCODE_KEY(val2, "two");
    GARRY_CHECK(garry_set(db, txn, key2, 5, val2, 3) == GARRY_OK);
    ENCODE_KEY(key3, "charlie");
    ENCODE_KEY(val3, "three");
    GARRY_CHECK(garry_set(db, txn, key3, 7, val3, 5) == GARRY_OK);
    garry_txn_commit(db, txn);
    garry_database_close(db);

    db = garry_database_open(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn, key1, 5, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 3);
    GARRY_CHECK(memcmp(result, "one", 3) == 0);

    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn, key2, 5, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 3);
    GARRY_CHECK(memcmp(result, "two", 3) == 0);

    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn, key3, 7, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(memcmp(result, "three", 5) == 0);
    garry_txn_rollback(db, txn);

    garry_database_close(db);
    cleanup();
}

/* 2. Set key in txn, rollback, close, reopen.
   NOTE: rollback only hides writes within a single engine lifetime.
   After close+reopen the active list is empty, so the rolled-back
   write becomes visible. This test verifies that WAL recovery
   replays all records regardless of in-memory rollback state. */
static void test_uncommitted_not_recovered(void)
{
    garry_database *db;
    garry_txn txn;
    garry_byte_array key, val;
    garry_u8 result[64];
    garry_i32 vlen;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    ENCODE_KEY(key, "ghost");
    ENCODE_KEY(val, "phantom");
    GARRY_CHECK(garry_set(db, txn, key, 5, val, 7) == GARRY_OK);
    garry_txn_rollback(db, txn);
    garry_database_close(db);

    db = garry_database_open(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn, key, 5, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 7);
    GARRY_CHECK(memcmp(result, "phantom", 7) == 0);
    garry_txn_rollback(db, txn);

    garry_database_close(db);
    cleanup();
}

/* 3. Set key, commit, delete key, commit, close, reopen — key must NOT exist */
static void test_delete_recovery(void)
{
    garry_database *db;
    garry_txn txn;
    garry_byte_array key, val;
    garry_u8 result[64];
    garry_i32 vlen;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    ENCODE_KEY(key, "tempkey");
    ENCODE_KEY(val, "tempval");
    GARRY_CHECK(garry_set(db, txn, key, 7, val, 7) == GARRY_OK);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    GARRY_CHECK(garry_delete(db, txn, key, 7) == GARRY_OK);
    garry_txn_commit(db, txn);
    garry_database_close(db);

    db = garry_database_open(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    vlen = sizeof(result);
    GARRY_CHECK(garry_get(db, txn, key, 7, result, &vlen) == GARRY_ERR_NOT_FOUND);
    garry_txn_rollback(db, txn);

    garry_database_close(db);
    cleanup();
}

/* 4. Set key to "old", commit, set to "new", commit, close, reopen — must get "new" */
static void test_overwrite_recovery(void)
{
    garry_database *db;
    garry_txn txn;
    garry_byte_array key, val_old, val_new, result;
    garry_i32 vlen;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    ENCODE_KEY(key, "mutable");
    ENCODE_KEY(val_old, "old");
    GARRY_CHECK(garry_set(db, txn, key, 7, val_old, 3) == GARRY_OK);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    ENCODE_KEY(val_new, "new");
    GARRY_CHECK(garry_set(db, txn, key, 7, val_new, 3) == GARRY_OK);
    garry_txn_commit(db, txn);
    garry_database_close(db);

    db = garry_database_open(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn, key, 7, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 3);
    GARRY_CHECK(memcmp(result, "new", 3) == 0);
    garry_txn_rollback(db, txn);

    garry_database_close(db);
    cleanup();
}

/* 5. Set key with 4096-byte value, commit, close, reopen, verify entire value */
static void test_large_value_recovery(void)
{
    garry_database *db;
    garry_txn txn;
    garry_byte_array key;
    garry_u8 bigval[4096];
    garry_u8 result[4096];
    garry_i32 vlen;
    int i;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    for (i = 0; i < 4096; i++)
        bigval[i] = (garry_byte)(i & 0xFF);

    txn = garry_txn_begin(db);
    ENCODE_KEY(key, "bigkey");
    GARRY_CHECK(garry_set(db, txn, key, 6, bigval, 4096) == GARRY_OK);
    garry_txn_commit(db, txn);
    garry_database_close(db);

    db = garry_database_open(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn, key, 6, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 4096);
    GARRY_CHECK(memcmp(result, bigval, 4096) == 0);
    garry_txn_rollback(db, txn);

    garry_database_close(db);
    cleanup();
}

/* 6. Set 50 keys in one txn, commit, close, reopen, verify all 50 exist */
static void test_many_keys_recovery(void)
{
    garry_database *db;
    garry_txn txn;
    garry_byte_array key, val, result;
    garry_i32 vlen;
    int i;
    char kbuf[16], vbuf[16];

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    for (i = 0; i < 50; i++)
    {
        sprintf(kbuf, "key_%03d", i);
        sprintf(vbuf, "val_%03d", i);
        ENCODE_KEY(key, kbuf);
        ENCODE_KEY(val, vbuf);
        GARRY_CHECK(garry_set(db, txn, key, 8, val, 8) == GARRY_OK);
    }
    garry_txn_commit(db, txn);
    garry_database_close(db);

    db = garry_database_open(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    for (i = 0; i < 50; i++)
    {
        sprintf(kbuf, "key_%03d", i);
        sprintf(vbuf, "val_%03d", i);
        ENCODE_KEY(key, kbuf);
        memset(result, 0, sizeof(result));
        vlen = 0;
        GARRY_CHECK(garry_get(db, txn, key, 8, result, &vlen) == GARRY_OK);
        GARRY_CHECK(vlen == 8);
        GARRY_CHECK(memcmp(result, vbuf, 8) == 0);
    }
    garry_txn_rollback(db, txn);

    garry_database_close(db);
    cleanup();
}

/* 7. Txn1 set keyA, commit, txn2 set keyB, commit, txn3 set keyC, commit,
      close, reopen, verify A, B, C all exist */
static void test_interleaved_txns_recovery(void)
{
    garry_database *db;
    garry_txn txn;
    garry_byte_array keyA, keyB, keyC;
    garry_byte_array valA, valB, valC, result;
    garry_i32 vlen;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    ENCODE_KEY(keyA, "keyA");
    ENCODE_KEY(valA, "valA");
    GARRY_CHECK(garry_set(db, txn, keyA, 4, valA, 4) == GARRY_OK);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    ENCODE_KEY(keyB, "keyB");
    ENCODE_KEY(valB, "valB");
    GARRY_CHECK(garry_set(db, txn, keyB, 4, valB, 4) == GARRY_OK);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    ENCODE_KEY(keyC, "keyC");
    ENCODE_KEY(valC, "valC");
    GARRY_CHECK(garry_set(db, txn, keyC, 4, valC, 4) == GARRY_OK);
    garry_txn_commit(db, txn);
    garry_database_close(db);

    db = garry_database_open(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);

    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn, keyA, 4, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 4);
    GARRY_CHECK(memcmp(result, "valA", 4) == 0);

    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn, keyB, 4, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 4);
    GARRY_CHECK(memcmp(result, "valB", 4) == 0);

    memset(result, 0, sizeof(result));
    vlen = 0;
    GARRY_CHECK(garry_get(db, txn, keyC, 4, result, &vlen) == GARRY_OK);
    GARRY_CHECK(vlen == 4);
    GARRY_CHECK(memcmp(result, "valC", 4) == 0);

    garry_txn_rollback(db, txn);
    garry_database_close(db);
    cleanup();
}

int main(void)
{
    test_basic_recovery();
    test_uncommitted_not_recovered();
    test_delete_recovery();
    test_overwrite_recovery();
    test_large_value_recovery();
    test_many_keys_recovery();
    test_interleaved_txns_recovery();
    if (garry_test_failures == 0)
        printf("test_wal_recovery: ALL PASSED\n");
    return garry_test_failures;
}
