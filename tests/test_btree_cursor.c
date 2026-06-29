/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/types.h"
#include "storage_types.h"
#include "storage_core.h"
#include "buffer_pool.h"
#include "btree_node.h"
#include "btree_search.h"
#include "btree_modify.h"
#include "btree_cursor.h"
#include "key_encoding.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

static void test_cursor_empty_tree(void)
{
    garry_buffer_pool *pool;
    garry_i32 root;
    garry_btree_cursor_handle cur;
    garry_byte_array k;
    garry_u32 kl;

    pool = garry_pool_create("tcurs1.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    root = garry_allocate_leaf(pool);
    GARRY_CHECK(root >= 0);

    cur = garry_btree_cursor_open(pool, root, NULL, 0);
    GARRY_CHECK(!garry_btree_cursor_next(pool, &cur, &k, &kl));

    garry_btree_cursor_close(&cur);
    garry_pool_close(pool);
}

static void test_cursor_iterates_all(void)
{
    garry_buffer_pool *pool;
    garry_i32 root;
    garry_byte_array k, v;
    garry_btree_cursor_handle cur;
    garry_u32 count = 0;
    garry_u32 kl;

    pool = garry_pool_create("tcurs2.db", 8, 4096);
    GARRY_CHECK(pool != NULL);
    root = garry_allocate_leaf(pool);
    GARRY_CHECK(root >= 0);

    ENCODE_KEY(k, "a"); ENCODE_KEY(v, "va");
    garry_btree_insert(pool, &root, k, 1, v, 2);
    ENCODE_KEY(k, "b"); ENCODE_KEY(v, "vb");
    garry_btree_insert(pool, &root, k, 1, v, 2);
    ENCODE_KEY(k, "c"); ENCODE_KEY(v, "vc");
    garry_btree_insert(pool, &root, k, 1, v, 2);

    cur = garry_btree_cursor_open(pool, root, NULL, 0);
    while (garry_btree_cursor_next(pool, &cur, &k, &kl)) count++;
    GARRY_CHECK(count == 3);

    garry_btree_cursor_close(&cur);
    garry_pool_close(pool);
}

static void test_cursor_peek(void)
{
    garry_buffer_pool *pool;
    garry_i32 root;
    garry_byte_array k, v, pk;
    garry_u32 pklen;
    garry_btree_cursor_handle cur;

    pool = garry_pool_create("tcurs3.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    root = garry_allocate_leaf(pool);
    GARRY_CHECK(root >= 0);

    ENCODE_KEY(k, "x"); ENCODE_KEY(v, "vx");
    garry_btree_insert(pool, &root, k, 1, v, 2);

    cur = garry_btree_cursor_open(pool, root, NULL, 0);
    GARRY_CHECK(garry_btree_cursor_peek(pool, &cur, &pk, &pklen));
    GARRY_CHECK(pklen == 1);
    GARRY_CHECK(pk[0] == 'x');

    garry_btree_cursor_close(&cur);
    garry_pool_close(pool);
}

static void test_cursor_value(void)
{
    garry_buffer_pool *pool;
    garry_i32 root;
    garry_byte_array k, v, cv;
    garry_u32 kl;
    garry_btree_cursor_handle cur;

    pool = garry_pool_create("tcurs4.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    root = garry_allocate_leaf(pool);
    GARRY_CHECK(root >= 0);

    ENCODE_KEY(k, "m"); ENCODE_KEY(v, "val_m");
    garry_btree_insert(pool, &root, k, 1, v, 5);

    cur = garry_btree_cursor_open(pool, root, NULL, 0);
    GARRY_CHECK(garry_btree_cursor_next(pool, &cur, &k, &kl));
    garry_btree_cursor_value(&cur, &cv);
    GARRY_CHECK(cv[0] == 'v');
    GARRY_CHECK(cv[1] == 'a');
    GARRY_CHECK(cv[2] == 'l');
    GARRY_CHECK(cv[3] == '_');
    GARRY_CHECK(cv[4] == 'm');

    garry_btree_cursor_close(&cur);
    garry_pool_close(pool);
}

static int test_cursor_prefix_filter(void) {
    garry_buffer_pool *pool;
    garry_i32 root;
    garry_byte_array k, v, pk;
    garry_u32 kl;
    garry_btree_cursor_handle cur;
    garry_u32 count = 0;
    pool = garry_pool_create("tcurs5.db", 8, 4096);
    root = garry_allocate_leaf(pool);
    ENCODE_KEY(k, "ab"); ENCODE_KEY(v, "v1");
    garry_btree_insert(pool, &root, k, 2, v, 2);
    ENCODE_KEY(k, "ac"); ENCODE_KEY(v, "v2");
    garry_btree_insert(pool, &root, k, 2, v, 2);
    ENCODE_KEY(k, "bb"); ENCODE_KEY(v, "v3");
    garry_btree_insert(pool, &root, k, 2, v, 2);
    ENCODE_KEY(k, "a");
    cur = garry_btree_cursor_open(pool, root, &k, 1);
    while (garry_btree_cursor_next(pool, &cur, &pk, &kl)) count++;
    GARRY_CHECK(count == 2);
    garry_btree_cursor_close(&cur);
    garry_pool_close(pool);
    return 0;
}

int main(void)
{
    test_cursor_empty_tree();
    test_cursor_iterates_all();
    test_cursor_peek();
    test_cursor_value();
    test_cursor_prefix_filter();
    if (garry_test_failures == 0) printf("test_btree_cursor: ALL PASSED\n");
    return garry_test_failures;
}
