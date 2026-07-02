/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "garry/version.h"
#include "test_helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_DB "/tmp/garry_version_iter_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static void test_iter_returns_three_newest_first(void)
{
    garry_database *db;
    garry_txn txn;
    garry_byte_array key;
    garry_u8 value[256];
    int i;

    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    for (i = 0; i < 3; i++)
    {
        txn = garry_txn_begin(db);
        ENCODE_KEY(key, "k");
        sprintf((char *)value, "v%d", i);
        GARRY_CHECK(garry_set(db, txn, key, 1, value, 2) == GARRY_OK);
        garry_txn_commit(db, txn);
    }

    txn = garry_txn_begin(db);
    {
        garry_version_iter *it;
        garry_i32 txid;
        garry_u8 *val;
        garry_i32 vlen;
        garry_bool tomb;

        ENCODE_KEY(key, "k");
        it = garry_version_iter_open(db, txn, key, 1);
        GARRY_CHECK(it != NULL);

        GARRY_CHECK(garry_version_iter_next(it, &txid, &val, &vlen, &tomb) == GARRY_TRUE);
        GARRY_CHECK(tomb == GARRY_FALSE);
        GARRY_CHECK(vlen == 2);
        GARRY_CHECK(memcmp(val, "v2", 2) == 0);
        free(val);

        GARRY_CHECK(garry_version_iter_next(it, &txid, &val, &vlen, &tomb) == GARRY_TRUE);
        GARRY_CHECK(memcmp(val, "v1", 2) == 0);
        free(val);

        GARRY_CHECK(garry_version_iter_next(it, &txid, &val, &vlen, &tomb) == GARRY_TRUE);
        GARRY_CHECK(memcmp(val, "v0", 2) == 0);
        free(val);

        GARRY_CHECK(garry_version_iter_next(it, &txid, &val, &vlen, &tomb) == GARRY_FALSE);
        garry_version_iter_close(it);
    }
    garry_txn_commit(db, txn);

    garry_database_close(db);
    cleanup();
}

int main(void)
{
    test_iter_returns_three_newest_first();
    if (garry_test_failures == 0)
        printf("test_version_iter: ALL PASSED\n");
    return garry_test_failures;
}
