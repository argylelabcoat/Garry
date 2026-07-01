#include "garry/api.h"
#include "storage_engine.h"
#include "storage_txn.h"
#include "storage_ops.h"
#include "storage_cursor.h"
#include "engine_settings.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

#define TEST_DB "/tmp/garry_cursor_order_test.db"

static void cleanup(void)
{
    remove(TEST_DB);
    remove(TEST_DB "-wal");
    remove(TEST_DB "-wal-wal");
    remove(TEST_DB "-wal.ckpt");
    remove(TEST_DB ".wal");
    remove(TEST_DB ".ckpt");
}

static void test_cursor_returns_keys_in_sorted_order(void)
{
    garry_engine_handle *eng;
    garry_txn_id txn;
    garry_byte key[512], value[512];
    garry_byte ck[512], cv[512];
    garry_i32 klen, vlen;
    garry_storage_cursor cur;
    garry_i32 count = 0;
    garry_byte prev_key[512];
    garry_i32 prev_klen = 0;

    printf("test_cursor_returns_keys_in_sorted_order\n");
    cleanup();
    eng = garry_storage_init(TEST_DB, garry_default_engine_settings());
    GARRY_CHECK(eng != NULL);

    /* Insert keys in reverse/alphabetical order */
    txn = garry_storage_begin(eng);
    
    ENCODE_KEY(key, "charlie");
    ENCODE_KEY(value, "vc");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 7, value, 2));
    
    ENCODE_KEY(key, "alice");
    ENCODE_KEY(value, "va");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 5, value, 2));
    
    ENCODE_KEY(key, "bob");
    ENCODE_KEY(value, "vb");
    GARRY_CHECK(garry_storage_set(eng, txn, key, 3, value, 2));
    
    garry_storage_commit(eng, txn);

    /* Iterate with cursor and check order */
    txn = garry_storage_begin(eng);
    cur = garry_storage_cursor_open(eng, txn, NULL, 0);
    
    prev_klen = 0;
    while (garry_storage_cursor_next(&cur, ck, &klen, cv, &vlen))
    {
        if (prev_klen > 0)
        {
            /* Current key should be >= previous key */
            GARRY_CHECK(garry_byte_compare(ck, klen, prev_key, prev_klen) >= 0);
        }
        memcpy(prev_key, ck, sizeof(garry_byte_array));
        prev_klen = klen;
        count++;
    }
    garry_storage_cursor_close(&cur);
    
    GARRY_CHECK(count == 3);
    garry_storage_rollback(eng, txn);

    garry_storage_close(eng);
    cleanup();
}

int main(void)
{
    test_cursor_returns_keys_in_sorted_order();
    if (garry_test_failures == 0)
        printf("test_cursor_order: ALL PASSED\n");
    return garry_test_failures;
}
