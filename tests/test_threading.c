/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "test_helpers.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_thread_test.db"
#define NUM_THREADS 4
#define OPS_PER_THREAD 100

typedef struct {
    garry_database *db;
    int thread_id;
} thread_arg;

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static void* reader_thread(void *arg)
{
    thread_arg *ta = (thread_arg *)arg;
    int i;

    for (i = 0; i < OPS_PER_THREAD; i++) {
        garry_txn txn;
        garry_u8 key[512], result[512];
        garry_i32 vlen;
        garry_status_t ok;
        int idx = i % 26;
        char c = (char)('a' + idx);

        memset(key, 0, sizeof(key));
        memset(result, 0, sizeof(result));
        key[0] = (garry_u8)c;

        txn = garry_txn_begin(ta->db);
        if (txn <= 0) continue;
        ok = garry_get(ta->db, txn, key, 1, result, &vlen);
        (void)ok;
        garry_txn_commit(ta->db, txn);
    }

    return NULL;
}

static void* writer_thread(void *arg)
{
    thread_arg *ta = (thread_arg *)arg;
    int i;

    for (i = 0; i < OPS_PER_THREAD; i++) {
        garry_txn txn;
        garry_u8 key[512], value[512];
        garry_status_t ok;
        int idx = ta->thread_id * OPS_PER_THREAD + i;
        char c = (char)('A' + (idx % 26));

        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
        key[0] = (garry_u8)c;
        value[0] = (garry_u8)c;
        value[1] = (garry_u8)'_';
        value[2] = (garry_u8)'v';

        txn = garry_txn_begin(ta->db);
        if (txn <= 0) continue;
        ok = garry_set(ta->db, txn, key, 1, value, 3);
        if (ok != GARRY_OK) {
            garry_txn_rollback(ta->db, txn);
            continue;
        }
        garry_txn_commit(ta->db, txn);
    }

    return NULL;
}

static void test_concurrent_readers(void)
{
    garry_database *db;
    garry_txn txn;
    pthread_t threads[NUM_THREADS];
    thread_arg args[NUM_THREADS];
    int i, ret;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    for (i = 0; i < 26; i++) {
        garry_u8 key[512], value[512];
        garry_status_t ok;

        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
        key[0] = (garry_u8)('a' + i);
        value[0] = (garry_u8)('a' + i);
        value[1] = (garry_u8)'_';
        value[2] = (garry_u8)'v';
        value[3] = (garry_u8)'a';
        value[4] = (garry_u8)'l';

        txn = garry_txn_begin(db);
        GARRY_CHECK(txn > 0);
        ok = garry_set(db, txn, key, 1, value, 5);
        GARRY_CHECK(ok == GARRY_OK);
        garry_txn_commit(db, txn);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        args[i].db = db;
        args[i].thread_id = i;
        ret = pthread_create(&threads[i], NULL, reader_thread, &args[i]);
        GARRY_CHECK(ret == 0);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        ret = pthread_join(threads[i], NULL);
        GARRY_CHECK(ret == 0);
    }

    garry_database_close(db);
    cleanup();
    printf("test_concurrent_readers: PASSED\n");
}

static void test_concurrent_writers(void)
{
    garry_database *db;
    pthread_t threads[NUM_THREADS];
    thread_arg args[NUM_THREADS];
    int i, ret;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    for (i = 0; i < NUM_THREADS; i++) {
        args[i].db = db;
        args[i].thread_id = i;
        ret = pthread_create(&threads[i], NULL, writer_thread, &args[i]);
        GARRY_CHECK(ret == 0);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        ret = pthread_join(threads[i], NULL);
        GARRY_CHECK(ret == 0);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        int j;
        for (j = 0; j < OPS_PER_THREAD; j++) {
            garry_txn txn;
            garry_u8 key[512], result[512];
            garry_i32 vlen;
            garry_status_t ok;
            int idx = i * OPS_PER_THREAD + j;
            char c = (char)('A' + (idx % 26));

            memset(key, 0, sizeof(key));
            memset(result, 0, sizeof(result));
            key[0] = (garry_u8)c;

            txn = garry_txn_begin(db);
            GARRY_CHECK(txn > 0);
            ok = garry_get(db, txn, key, 1, result, &vlen);
            GARRY_CHECK(ok == GARRY_OK);
            GARRY_CHECK(vlen == 3);
            GARRY_CHECK(result[0] == (garry_u8)c);
            GARRY_CHECK(result[1] == (garry_u8)'_');
            GARRY_CHECK(result[2] == (garry_u8)'v');
            garry_txn_commit(db, txn);
        }
    }

    garry_database_close(db);
    cleanup();
    printf("test_concurrent_writers: PASSED\n");
}

static void test_concurrent_mixed(void)
{
    garry_database *db;
    garry_txn txn;
    pthread_t threads[NUM_THREADS];
    thread_arg args[NUM_THREADS];
    int i, ret;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    for (i = 0; i < 26; i++) {
        garry_u8 key[512], value[512];
        garry_status_t ok;

        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
        key[0] = (garry_u8)('a' + i);
        value[0] = (garry_u8)('a' + i);
        value[1] = (garry_u8)'_';
        value[2] = (garry_u8)'v';

        txn = garry_txn_begin(db);
        GARRY_CHECK(txn > 0);
        ok = garry_set(db, txn, key, 1, value, 3);
        GARRY_CHECK(ok == GARRY_OK);
        garry_txn_commit(db, txn);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        args[i].db = db;
        args[i].thread_id = i;
        if (i < 2) {
            ret = pthread_create(&threads[i], NULL, reader_thread, &args[i]);
        } else {
            ret = pthread_create(&threads[i], NULL, writer_thread, &args[i]);
        }
        GARRY_CHECK(ret == 0);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        ret = pthread_join(threads[i], NULL);
        GARRY_CHECK(ret == 0);
    }

    garry_database_close(db);
    cleanup();
    printf("test_concurrent_mixed: PASSED\n");
}

int main(void)
{
    test_concurrent_readers();
    test_concurrent_writers();
    test_concurrent_mixed();

    if (garry_test_failures == 0) printf("test_threading: ALL PASSED\n");
    return garry_test_failures;
}
