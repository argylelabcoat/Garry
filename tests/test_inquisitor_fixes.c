/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/api.h"
#include "garry/keysplit.h"
#include "key_encoding.h"
#include "record_codec.h"
#include "slotted_page.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "/tmp/garry_inquisitor_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static void test_garry_data_returns(void)
{
    garry_database *db;
    garry_txn txn;
    garry_i32 result;
    garry_u8 key[512];
    garry_i32 klen;
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);
    GARRY_CHECK(txn != 0);
    klen = garry_make_key("mykey", key);
    GARRY_CHECK(klen > 0);
    garry_set(db, txn, key, klen, (const garry_u8*)"val", 3);
    garry_txn_commit(db, txn);
    txn = garry_txn_begin(db);
    result = garry_data(db, txn, key, klen);
    GARRY_CHECK(result == GARRY_DATA_HAS_VALUE);
    garry_txn_rollback(db, txn);
    txn = garry_txn_begin(db);
    klen = garry_make_key("noexist", key);
    result = garry_data(db, txn, key, klen);
    GARRY_CHECK(result == GARRY_DATA_NOT_FOUND);
    garry_txn_rollback(db, txn);
    garry_database_close(db);
}

static void test_tuple_length(void)
{
    garry_key_tuple t;
    garry_i32 len;
    t = garry_make_key_3("a", "b", "c");
    len = garry_tuple_length(&t);
    /* Each part "a","b","c" is 1 byte, each has 1-byte length prefix = (1+1)*3 = 6 */
    GARRY_CHECK(len == 6);
    t = garry_make_key_2("hello", "world");
    len = garry_tuple_length(&t);
    GARRY_CHECK(len == (1+5)+(1+5));
}

static void test_page_get_validation(void)
{
    garry_page_buffer buf;
    garry_byte_array data;
    garry_i32 idx, rlen;
    garry_page_init(buf, GARRY_NODE_LEAF, 0, 4096);
    data[0] = 65; data[1] = 66; data[2] = 67;
    idx = garry_page_insert(buf, data, 3, 4096);
    GARRY_CHECK(idx == 0);
    memset(data, 0, sizeof(data));
    rlen = garry_page_get(&buf, 99, data, 4096);
    GARRY_CHECK(rlen == -1);
}

static void test_decode_kv_validation(void)
{
    garry_byte key_out[256], val_out[256];
    garry_i32 klen, vlen;
    garry_decode_kv(NULL, 0, key_out, &klen, val_out, &vlen);
    GARRY_CHECK(klen == 0);
    GARRY_CHECK(vlen == 0);
}

static void test_integer_subscript_roundtrip(void)
{
    garry_byte enc[256];
    garry_i32 dec;
    garry_i32 vals[3] = {42, -10, 300};
    int i;
    for (i = 0; i < 3; i++) {
        garry_encode_integer_subscript(vals[i], enc);
        dec = garry_decode_integer_subscript(enc, 0);
        GARRY_CHECK(dec == vals[i]);
    }
}

int main(void)
{
    test_garry_data_returns();
    test_tuple_length();
    test_page_get_validation();
    test_decode_kv_validation();
    test_integer_subscript_roundtrip();
    if (garry_test_failures == 0) printf("test_inquisitor_fixes: ALL PASSED\n");
    return garry_test_failures;
}
