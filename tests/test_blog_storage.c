/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file test_blog_storage.c
 * @brief Integration test simulating a blog storage pattern with nested keys.
 *
 * Uses garry_key_split to encode '/' delimited paths into the internal
 * length-prefixed key format, exercising the real hierarchical key path.
 */

#include "garry/api.h"
#include "garry/keysplit.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_blog_test.db"

#ifndef GARRY_TEST_CONTENT_DIR
#define GARRY_TEST_CONTENT_DIR "content"
#endif

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static void test_store_and_retrieve_post(void)
{
    garry_database *db;
    garry_txn txn;
    garry_byte_array key;
    garry_u8 value[256], result[256];
    garry_i32 klen, vlen;
    garry_bool ok;
    char fpath[512];

    printf("test_store_and_retrieve_post\n");
    cleanup();

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);

    /* Store title */
    klen = garry_key_split("post/zen-algo/title", '/', key);
    ok = garry_set(db, txn, key, klen, (const garry_u8*)"Zen Garden", 10);
    GARRY_CHECK(ok == GARRY_OK);

    /* Store author */
    klen = garry_key_split("post/zen-algo/author", '/', key);
    ok = garry_set(db, txn, key, klen, (const garry_u8*)"Byte Gardener", 13);
    GARRY_CHECK(ok == GARRY_OK);

    /* Store date */
    klen = garry_key_split("post/zen-algo/date", '/', key);
    ok = garry_set(db, txn, key, klen, (const garry_u8*)"2026-05-01", 10);
    GARRY_CHECK(ok == GARRY_OK);

    /* Store content (first 200 bytes of file) */
    sprintf(fpath, "%s/zen-and-the-algorithmic-garden.md", GARRY_TEST_CONTENT_DIR);
    memset(value, 0, sizeof(value));
    {
        FILE *f = fopen(fpath, "rb");
        long flen;
        GARRY_CHECK(f != NULL);
        flen = (long)fread(value, 1, 200, f);
        fclose(f);
        klen = garry_key_split("post/zen-algo/content", '/', key);
        ok = garry_set(db, txn, key, klen, value, (garry_i32)flen);
        GARRY_CHECK(ok == GARRY_OK);
    }

    garry_txn_commit(db, txn);

    /* Verify */
    txn = garry_txn_begin(db);

    klen = garry_key_split("post/zen-algo/title", '/', key);
    memset(result, 0, sizeof(result));
    vlen = 0;
    ok = garry_get(db, txn, key, klen, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == 10);
    GARRY_CHECK(memcmp(result, "Zen Garden", 10) == 0);

    klen = garry_key_split("post/zen-algo/author", '/', key);
    memset(result, 0, sizeof(result));
    vlen = 0;
    ok = garry_get(db, txn, key, klen, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == 13);
    GARRY_CHECK(memcmp(result, "Byte Gardener", 13) == 0);

    garry_txn_rollback(db, txn);

    garry_database_close(db);
    cleanup();
}

int main(void)
{
    test_store_and_retrieve_post();
    if (garry_test_failures == 0) printf("test_blog_storage: ALL PASSED\n");
    return garry_test_failures;
}
