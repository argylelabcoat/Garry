/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[512], value[512], result[512];
    garry_i32 vlen;
    garry_status_t ok;

    db = garry_database_create("/tmp/garry_pub_test.db");
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);
    GARRY_CHECK(txn > 0);
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));
    key[0] = 'h'; key[1] = 'e'; key[2] = 'l'; key[3] = 'l'; key[4] = 'o';
    value[0] = 'w'; value[1] = 'o'; value[2] = 'r'; value[3] = 'l'; value[4] = 'd';
    ok = garry_set(db, txn, key, 5, value, 5);
    GARRY_CHECK(ok == GARRY_OK);
    garry_txn_commit(db, txn);
    txn = garry_txn_begin(db);
    GARRY_CHECK(txn > 0);
    memset(result, 0, sizeof(result));
    vlen = 0;
    ok = garry_get(db, txn, key, 5, result, &vlen);
    GARRY_CHECK(ok == GARRY_OK);
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(result[0] == 'w');
    GARRY_CHECK(result[4] == 'd');
    garry_txn_rollback(db, txn);
    garry_database_close(db);
    if (garry_test_failures == 0) printf("test_public_api: ALL PASSED\n");
    return garry_test_failures;
}
