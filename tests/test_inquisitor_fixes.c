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
#include "storage_types.h"
#include "version_chain.h"
#include "lock_table.h"
#include "btree_cursor.h"
#include "util_endian.h"
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

static void test_decode_kv_returns_bool(void)
{
    garry_byte key_out[256], val_out[256];
    garry_i32 klen, vlen;
    garry_bool ok;

    ok = garry_decode_kv(NULL, 0, key_out, &klen, val_out, &vlen);
    GARRY_CHECK(ok == GARRY_FALSE);
    GARRY_CHECK(klen == 0);
    GARRY_CHECK(vlen == 0);

    ok = garry_decode_kv((const garry_byte*)"x", 1, key_out, &klen, val_out, &vlen);
    GARRY_CHECK(ok == GARRY_FALSE);
    GARRY_CHECK(klen == 0);

    {
        garry_byte buf[256];
        garry_i32 len;
        len = garry_encode_kv((const garry_byte*)"k", 1, (const garry_byte*)"v", 1, buf);
        ok = garry_decode_kv(buf, len, key_out, &klen, val_out, &vlen);
        GARRY_CHECK(ok == GARRY_TRUE);
        GARRY_CHECK(klen == 1);
        GARRY_CHECK(vlen == 1);
        GARRY_CHECK(key_out[0] == 'k');
        GARRY_CHECK(val_out[0] == 'v');
    }
}

static void test_util_endian_roundtrip(void)
{
    garry_byte buf[8];
    garry_i32 vals[4] = {0, 1, 0x7FFFFFFF, -1};
    int i;
    for (i = 0; i < 4; i++) {
        garry_write_le32(buf, 0, vals[i]);
        GARRY_CHECK(garry_read_le32(buf, 0) == vals[i]);
    }
    garry_write_le32(buf, 2, 0x12345678);
    GARRY_CHECK(garry_read_le32(buf, 2) == 0x12345678);
}

static void test_version_chain_prune_preserves_on_full_page(void)
{
    garry_page_buffer buf;
    garry_i32 i;
    garry_txn_id active[1] = {0};
    garry_buffer_pool *pool;

    pool = garry_pool_create("/tmp/garry_prune_test.db", 4, 4096);
    GARRY_CHECK(pool != NULL);

    garry_page_init(buf, GARRY_NODE_CHAIN, 0, 4096);

    for (i = 1; i <= 20; i++) {
        garry_chain_page_append(buf, 4096, i, "x", 1);
    }

    garry_chain_page_prune(pool, buf, 4096, active, 1);

    GARRY_CHECK(garry_chain_page_has_version(buf, 4096, 1) == 0);
    GARRY_CHECK(garry_chain_page_has_version(buf, 4096, 20) == 1);

    garry_pool_close(pool);
    remove("/tmp/garry_prune_test.db");
}

/* CRITICAL FIX: version chain visibility — reader must see its own uncommitted writes */
static void test_visibility_sees_own_uncommitted_writes(void)
{
    garry_page_buffer buf;
    garry_i32 vlen;
    char *val;
    garry_txn_id active[1];

    garry_page_init(buf, GARRY_NODE_CHAIN, 0, 4096);
    garry_chain_page_append(buf, 4096, 5, "hello", 5);

    /* txn 5 is active (reading its own data) */
    active[0] = 5;
    val = garry_chain_page_find_visible(NULL, buf, 4096, 5, active, 1, &vlen);
    GARRY_CHECK(val != NULL);
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(memcmp(val, "hello", 5) == 0);
    free(val);
}

/* CRITICAL FIX: version chain visibility — other active txn's writes are hidden */
static void test_visibility_hides_other_active_txn_writes(void)
{
    garry_page_buffer buf;
    garry_i32 vlen;
    char *val;
    garry_txn_id active[2];

    garry_page_init(buf, GARRY_NODE_CHAIN, 0, 4096);
    garry_chain_page_append(buf, 4096, 5, "hello", 5);

    /* txn 5 wrote it, txn 10 is the reader. Both active. */
    active[0] = 5;
    active[1] = 10;
    val = garry_chain_page_find_visible(NULL, buf, 4096, 10, active, 2, &vlen);
    GARRY_CHECK(val == NULL);
}

/* CRITICAL FIX: version chain visibility — committed writes are visible */
static void test_visibility_shows_committed_writes(void)
{
    garry_page_buffer buf;
    garry_i32 vlen;
    char *val;
    garry_txn_id active[1];

    garry_page_init(buf, GARRY_NODE_CHAIN, 0, 4096);
    garry_chain_page_append(buf, 4096, 5, "hello", 5);

    /* txn 5 committed (not in active list), reader is txn 10 */
    active[0] = 10;
    val = garry_chain_page_find_visible(NULL, buf, 4096, 10, active, 1, &vlen);
    GARRY_CHECK(val != NULL);
    GARRY_CHECK(vlen == 5);
    GARRY_CHECK(memcmp(val, "hello", 5) == 0);
    free(val);
}

/* HIGH FIX: garry_config has max_key_size and max_subscripts fields */
static void test_config_has_new_fields(void)
{
    garry_config cfg;
    cfg = garry_config_default();
    GARRY_CHECK(cfg.max_key_size == GARRY_DEFAULT_MAX_KEY_SIZE);
    GARRY_CHECK(cfg.max_subscripts == GARRY_DEFAULT_MAX_SUBSCRIPTS);
    GARRY_CHECK(cfg.max_versions == GARRY_DEFAULT_MAX_VERSIONS);
    GARRY_CHECK(cfg.pool_size == GARRY_DEFAULT_POOL_SIZE);
    GARRY_CHECK(cfg.page_size == GARRY_DEFAULT_PAGE_SIZE);
}

/* HIGH FIX: O(1) count via key_count */
static void test_count_is_o1(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[256];
    garry_i32 klen, count;
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);

    txn = garry_txn_begin(db);
    klen = garry_make_key("k1", key);
    garry_set(db, txn, key, klen, (const garry_u8*)"v1", 2);
    klen = garry_make_key("k2", key);
    garry_set(db, txn, key, klen, (const garry_u8*)"v2", 2);
    klen = garry_make_key("k3", key);
    garry_set(db, txn, key, klen, (const garry_u8*)"v3", 2);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    count = garry_count(db, txn);
    GARRY_CHECK(count == 3);
    garry_txn_rollback(db, txn);

    /* Update existing key — count should not change */
    txn = garry_txn_begin(db);
    klen = garry_make_key("k1", key);
    garry_set(db, txn, key, klen, (const garry_u8*)"v1_new", 6);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    count = garry_count(db, txn);
    GARRY_CHECK(count == 3);
    garry_txn_rollback(db, txn);

    garry_database_close(db);
    cleanup();
}

static void test_btree_delete_root_shrink(void)
{
    garry_database *db;
    garry_txn txn;
    garry_u8 key[256], value[256];
    garry_i32 klen, vlen;
    garry_status_t ok;
    garry_bool found;
    cleanup();
    db = garry_database_create(TEST_DB);
    GARRY_CHECK(db != NULL);
    txn = garry_txn_begin(db);
    GARRY_CHECK(txn != 0);

    klen = garry_make_key("a", key);
    garry_set(db, txn, key, klen, (const garry_u8*)"1", 1);
    klen = garry_make_key("b", key);
    garry_set(db, txn, key, klen, (const garry_u8*)"2", 1);
    garry_txn_commit(db, txn);

    txn = garry_txn_begin(db);
    klen = garry_make_key("a", key);
    ok = garry_delete(db, txn, key, klen);
    GARRY_CHECK(ok == GARRY_OK);

    klen = garry_make_key("b", key);
    memset(value, 0, sizeof(value));
    vlen = 0;
    found = garry_get(db, txn, key, klen, value, &vlen);
    GARRY_CHECK(found == GARRY_OK);
    GARRY_CHECK(vlen == 1);
    GARRY_CHECK(value[0] == '2');

    klen = garry_make_key("a", key);
    found = garry_get(db, txn, key, klen, value, &vlen);
    GARRY_CHECK(found == GARRY_ERR_NOT_FOUND);

    garry_txn_rollback(db, txn);
    garry_database_close(db);
    cleanup();
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

static void test_lock_release_on_commit(void)
{
    garry_lock_manager mgr;
    garry_bool ok;

    mgr = garry_create_lock_manager();

    garry_lock_acquire(&mgr, 1, (const garry_byte*)"key1", 4, GARRY_LOCK_EXCLUSIVE, &ok);
    GARRY_CHECK(ok == 1);

    GARRY_CHECK(garry_lock_held(&mgr, 1, (const garry_byte*)"key1", 4) == 1);

    garry_lock_release(&mgr, 1);

    GARRY_CHECK(garry_lock_held(&mgr, 1, (const garry_byte*)"key1", 4) == 0);

    garry_lock_acquire(&mgr, 2, (const garry_byte*)"key1", 4, GARRY_LOCK_EXCLUSIVE, &ok);
    GARRY_CHECK(ok == 1);

    garry_lock_release(&mgr, 2);
}

static void test_lock_release_on_rollback(void)
{
    garry_lock_manager mgr;
    garry_bool ok;

    mgr = garry_create_lock_manager();

    garry_lock_acquire(&mgr, 1, (const garry_byte*)"key1", 4, GARRY_LOCK_SHARED, &ok);
    GARRY_CHECK(ok == 1);

    garry_lock_acquire(&mgr, 2, (const garry_byte*)"key1", 4, GARRY_LOCK_EXCLUSIVE, &ok);
    GARRY_CHECK(ok == 0);

    garry_lock_release(&mgr, 1);

    garry_lock_acquire(&mgr, 2, (const garry_byte*)"key1", 4, GARRY_LOCK_EXCLUSIVE, &ok);
    GARRY_CHECK(ok == 1);

    garry_lock_release(&mgr, 2);
}

static void test_overflow_read_returns_bool(void)
{
    garry_buffer_pool *pool;
    garry_bool ok;
    char buf[64];

    pool = garry_pool_create("/tmp/garry_overflow_test.db", 4, 4096);
    GARRY_CHECK(pool != NULL);

    ok = garry_overflow_read(pool, -1, 10, buf);
    GARRY_CHECK(ok == GARRY_FALSE);

    garry_pool_close(pool);
    remove("/tmp/garry_overflow_test.db");
}

int main(void)
{
    test_garry_data_returns();
    test_tuple_length();
    test_page_get_validation();
    test_decode_kv_returns_bool();
    test_util_endian_roundtrip();
    test_version_chain_prune_preserves_on_full_page();
    test_visibility_sees_own_uncommitted_writes();
    test_visibility_hides_other_active_txn_writes();
    test_visibility_shows_committed_writes();
    test_config_has_new_fields();
    test_count_is_o1();
    test_btree_delete_root_shrink();
    test_integer_subscript_roundtrip();
    test_lock_release_on_commit();
    test_lock_release_on_rollback();
    test_overflow_read_returns_bool();
    if (garry_test_failures == 0) printf("test_inquisitor_fixes: ALL PASSED\n");
    return garry_test_failures;
}
