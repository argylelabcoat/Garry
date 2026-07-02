/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define TEST_DB "/tmp/garry_concurrent_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

#define NUM_THREADS 4
#define KEYS_PER_THREAD 20

/* Each thread writes its own set of keys. */
static void *writer_thread(void *arg)
{
    garry_database *db = (garry_database *)arg;
    garry_txn txn;
    garry_u8 key[16], value[16];
    int i, tid;
    char kbuf[16], vbuf[16];

    /* Derive a thread-specific ID from pthread_self */
    tid = (int)(uintptr_t)pthread_self() & 0xFF;

    txn = garry_txn_begin(db);
    for (i = 0; i < KEYS_PER_THREAD; i++)
    {
        sprintf(kbuf, "t%02d_k%02d", tid, i);
        sprintf(vbuf, "v%02d_%02d", tid, i);
        memcpy(key, kbuf, strlen(kbuf));
        memcpy(value, vbuf, strlen(vbuf));
        garry_set(db, txn, key, (garry_i32)strlen(kbuf), value, (garry_i32)strlen(vbuf));
    }
    garry_txn_commit(db, txn);
    return NULL;
}

/* Multiple threads writing different keys concurrently. */
static void test_concurrent_writes(void)
{
    garry_database *db;
    garry_txn txn;
    pthread_t threads[NUM_THREADS];
    garry_u8 key[16], value[16];
    garry_i32 vlen;
    int i, j, tid;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    for (i = 0; i < NUM_THREADS; i++)
        pthread_create(&threads[i], NULL, writer_thread, db);
    for (i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    /* Verify all keys from all threads */
    txn = garry_txn_begin(db);
    for (i = 0; i < NUM_THREADS; i++)
    {
        tid = (int)(uintptr_t)threads[i] & 0xFF;
        for (j = 0; j < KEYS_PER_THREAD; j++)
        {
            char kbuf[16], vbuf[16];
            sprintf(kbuf, "t%02d_k%02d", tid, j);
            sprintf(vbuf, "v%02d_%02d", tid, j);
            memcpy(key, kbuf, strlen(kbuf));
            GARRY_CHECK(garry_get(db, txn, key, (garry_i32)strlen(kbuf), value, &vlen) == GARRY_OK);
            GARRY_CHECK(vlen == (garry_i32)strlen(vbuf));
        }
    }
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

/* Multiple threads reading while one writes. */
static void test_concurrent_read_write(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[16], value[16], result[256];
    garry_i32 vlen;
    int i;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    /* Pre-populate */
    txn = garry_txn_begin(db);
    for (i = 0; i < 10; i++)
    {
        char kbuf[16], vbuf[16];
        sprintf(kbuf, "pre%02d", i);
        sprintf(vbuf, "val%02d", i);
        memcpy(key, kbuf, 5);
        memcpy(value, vbuf, 5);
        garry_set(db, txn, key, 5, value, 5);
    }
    garry_txn_commit(db, txn);

    /* Concurrent reads and one writer — must not crash */
    /* Writer adds new keys */
    txn = garry_txn_begin(db);
    for (i = 10; i < 20; i++)
    {
        char kbuf[16], vbuf[16];
        sprintf(kbuf, "new%02d", i);
        sprintf(vbuf, "val%02d", i);
        memcpy(key, kbuf, 5);
        memcpy(value, vbuf, 5);
        garry_set(db, txn, key, 5, value, 5);
    }
    garry_txn_commit(db, txn);

    /* Reader verifies pre-populated keys still exist */
    txn = garry_txn_begin(db);
    for (i = 0; i < 10; i++)
    {
        char kbuf[16];
        sprintf(kbuf, "pre%02d", i);
        memcpy(key, kbuf, 5);
        GARRY_CHECK(garry_get(db, txn, key, 5, result, &vlen) == GARRY_OK);
    }
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

/* Concurrent deletes — each thread deletes its own keys. */
static void test_concurrent_deletes(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[16];
    int i;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    /* Set up keys */
    txn = garry_txn_begin(db);
    for (i = 0; i < 20; i++)
    {
        char kbuf[16], vbuf[16];
        sprintf(kbuf, "del%02d", i);
        sprintf(vbuf, "v%02d", i);
        garry_set(db, txn, (garry_u8 *)kbuf, 5, (garry_u8 *)vbuf, 3);
    }
    garry_txn_commit(db, txn);

    /* Delete odd keys */
    txn = garry_txn_begin(db);
    for (i = 1; i < 20; i += 2)
    {
        char kbuf[16];
        sprintf(kbuf, "del%02d", i);
        garry_delete(db, txn, (garry_u8 *)kbuf, 5);
    }
    garry_txn_commit(db, txn);

    /* Verify: even keys exist, odd keys don't */
    txn = garry_txn_begin(db);
    for (i = 0; i < 20; i++)
    {
        char kbuf[16];
        sprintf(kbuf, "del%02d", i);
        memcpy(key, kbuf, 5);
        if (i % 2 == 0)
            GARRY_CHECK(garry_exists(db, txn, key, 5));
        else
            GARRY_CHECK(!garry_exists(db, txn, key, 5));
    }
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

int main(void)
{
    test_concurrent_writes();
    test_concurrent_read_write();
    test_concurrent_deletes();
    if (garry_test_failures == 0)
        printf("test_concurrency: ALL PASSED\n");
    return garry_test_failures;
}
