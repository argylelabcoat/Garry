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

#define TEST_DB "/tmp/garry_last_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

/* Simple first/last with a few keys. */
static void test_first_last_basic(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 klen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "aaa");
    ENCODE_KEY(value, "1");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 3, value, 1));
    ENCODE_KEY(key, "bbb");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 3, value, 1));
    ENCODE_KEY(key, "ccc");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 3, value, 1));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);

    /* first should return "aaa" */
    memset(result, 0, sizeof(result));
    GARRY_CHECK(garry_storage_first(eng, txn, result, &klen));
    GARRY_CHECK(klen == 3);
    GARRY_CHECK(memcmp(result, "aaa", 3) == 0);

    /* last should return "ccc" */
    memset(result, 0, sizeof(result));
    GARRY_CHECK(garry_storage_last(eng, txn, result, &klen));
    GARRY_CHECK(klen == 3);
    GARRY_CHECK(memcmp(result, "ccc", 3) == 0);

    garry_storage_rollback(eng, txn);
    garry_storage_close(eng);
    cleanup();
}

/* Insert enough keys to trigger B-tree leaf splits (max 2 per leaf).
   With 3 keys/node, inserting 6+ keys guarantees splits. last must
   still find the rightmost visible key. */
static void test_last_after_splits(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 klen;
    int i;
    char kbuf[16];

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    /* Insert 8 keys with same-length names to ensure sort order */
    txn = garry_storage_begin(eng);
    for (i = 0; i < 8; i++)
    {
        sprintf(kbuf, "key%02d", i);
        memcpy(key, kbuf, 5);
        ENCODE_KEY(value, "v");
        GARRY_CHECK(garry_storage_set(eng, txn, key, 5, value, 1));
    }
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);

    /* last should return "key07" */
    memset(result, 0, sizeof(result));
    GARRY_CHECK(garry_storage_last(eng, txn, result, &klen));
    GARRY_CHECK(klen == 5);
    GARRY_CHECK(memcmp(result, "key07", 5) == 0);

    /* first should return "key00" */
    memset(result, 0, sizeof(result));
    GARRY_CHECK(garry_storage_first(eng, txn, result, &klen));
    GARRY_CHECK(klen == 5);
    GARRY_CHECK(memcmp(result, "key00", 5) == 0);

    garry_storage_rollback(eng, txn);
    garry_storage_close(eng);
    cleanup();
}

/* Delete the last key — last must retreat to the previous key. */
static void test_last_after_delete(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 klen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "aaa");
    ENCODE_KEY(value, "1");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 3, value, 1));
    ENCODE_KEY(key, "bbb");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 3, value, 1));
    ENCODE_KEY(key, "ccc");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 3, value, 1));
    garry_storage_commit(eng, txn);

    /* Delete "ccc" */
    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "ccc");
    GARRY_CHECK(garry_storage_delete(eng, txn, key, 3));
    garry_storage_commit(eng, txn);

    /* last must now return "bbb" */
    txn = garry_storage_begin(eng);
    memset(result, 0, sizeof(result));
    GARRY_CHECK(garry_storage_last(eng, txn, result, &klen));
    GARRY_CHECK(klen == 3);
    GARRY_CHECK(memcmp(result, "bbb", 3) == 0);
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* Delete all keys — last must return false. */
static void test_last_empty_after_delete(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 klen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    ENCODE_KEY(key, "solo");
    ENCODE_KEY(value, "1");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 4, value, 1));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(garry_storage_delete(eng, txn, key, 4));
    garry_storage_commit(eng, txn);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(!garry_storage_last(eng, txn, result, &klen));
    GARRY_CHECK(!garry_storage_first(eng, txn, result, &klen));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* next_key / prev_key navigation across splits. */
static void test_next_prev_across_splits(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512], result[512];
    garry_i32 klen;
    int i;
    char kbuf[16];

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    /* Insert 6 keys to force splits */
    txn = garry_storage_begin(eng);
    for (i = 0; i < 6; i++)
    {
        sprintf(kbuf, "nav%01d", i);
        memcpy(key, kbuf, 4);
        GARRY_CHECK(garry_storage_set(eng, txn, key, 4, value, 1));
    }
    garry_storage_commit(eng, txn);

    /* Forward iteration via first + next_key */
    txn = garry_storage_begin(eng);
    memset(result, 0, sizeof(result));
    GARRY_CHECK(garry_storage_first(eng, txn, result, &klen));
    memcpy(key, result, (size_t)klen);
    i = 1;
    while (garry_storage_next_key(eng, txn, key, klen, result, &klen))
    {
        memcpy(key, result, (size_t)klen);
        i++;
    }
    GARRY_CHECK(i == 6);
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

/* Empty database — first/last must return false. */
static void test_first_last_empty(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte result[512];
    garry_i32 klen;

    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    txn = garry_storage_begin(eng);
    GARRY_CHECK(!garry_storage_first(eng, txn, result, &klen));
    GARRY_CHECK(!garry_storage_last(eng, txn, result, &klen));
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

int main(void)
{
    test_first_last_basic();
    test_last_after_splits();
    test_last_after_delete();
    test_last_empty_after_delete();
    test_next_prev_across_splits();
    test_first_last_empty();
    if (garry_test_failures == 0)
        printf("test_last_cursor: ALL PASSED\n");
    return garry_test_failures;
}
