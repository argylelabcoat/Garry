/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file test_blog_storage.c
 * @brief Integration test simulating a blog storage pattern with nested keys.
 */

#include "garry/api.h"
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

static long read_file(const char *path, char *buf, long max_len)
{
    FILE *f;
    long n;
    f = fopen(path, "rb");
    if (!f) return -1;
    n = (long)fread(buf, 1, (size_t)max_len, f);
    fclose(f);
    return n;
}

static void test_store_and_retrieve_post(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[256], value[256], result[256];
    garry_i32 vlen;
    garry_bool ok;
    char path[512];

    printf("test_store_and_retrieve_post\n");
    cleanup();

    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);

    /* Store title */
    sprintf((char*)key, "post/zen-algo/title");
    ok = garry_set(db, txn, key, 18, (const garry_u8*)"Zen Garden", 10);
    GARRY_CHECK(ok == GARRY_OK);

    /* Store author */
    sprintf((char*)key, "post/zen-algo/author");
    ok = garry_set(db, txn, key, 19, (const garry_u8*)"Byte Gardener", 13);
    GARRY_CHECK(ok == GARRY_OK);

    /* Store date */
    sprintf((char*)key, "post/zen-algo/date");
    ok = garry_set(db, txn, key, 17, (const garry_u8*)"2026-05-01", 10);
    GARRY_CHECK(ok == GARRY_OK);

    /* Store content (first 200 bytes of file) */
    sprintf(path, "%s/zen-and-the-algorithmic-garden.md", GARRY_TEST_CONTENT_DIR);
    memset(value, 0, sizeof(value));
    {
        FILE *f = fopen(path, "rb");
        GARRY_CHECK(f != NULL);
        {
            long flen = (long)fread(value, 1, 200, f);
            fclose(f);
            sprintf((char*)key, "post/zen-algo/content");
            ok = garry_set(db, txn, key, 18, value, (garry_i32)flen);
            GARRY_CHECK(ok == GARRY_OK);
        }
    }

    garry_txn_commit(db, txn);

    /* Verify */
    txn = garry_txn_begin(db);

    sprintf((char*)key, "post/zen-algo/title");
    memset(result, 0, sizeof(result));
    vlen = 0;
    ok = garry_get(db, txn, key, 18, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == 10);
    GARRY_CHECK(memcmp(result, "Zen Garden", 10) == 0);

    sprintf((char*)key, "post/zen-algo/author");
    memset(result, 0, sizeof(result));
    vlen = 0;
    ok = garry_get(db, txn, key, 19, result, &vlen);
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
