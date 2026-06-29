/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "btree_modify.h"
#include "btree_search.h"
#include "buffer_pool.h"
#include "test_helpers.h"

static void test_insert_single(void)
{
    garry_buffer_pool *pool;
    garry_i32 root;
    garry_byte key[512];
    garry_byte value[512];
    garry_btree_node node;

    pool = garry_pool_create("tm_insert1.db", 8, 4096);
    GARRY_CHECK(pool != NULL);
    root = garry_allocate_leaf(pool);
    GARRY_CHECK(root >= 0);

    ENCODE_KEY(key, "alpha");
    ENCODE_KEY(value, "val1");
    garry_btree_insert(pool, &root, key, 5, value, 4);

    garry_load_node(pool, root, &node);
    GARRY_CHECK(node.entry_count == 1);
    GARRY_CHECK(node.key_lens[0] == 5);
    GARRY_CHECK(memcmp(node.keys[0], "alpha", 5) == 0);
    GARRY_CHECK(node.value_lens[0] == 4);
    GARRY_CHECK(memcmp(node.values[0], "val1", 4) == 0);

    garry_pool_close(pool);
}

static void test_insert_sorted(void)
{
    garry_buffer_pool *pool;
    garry_i32 root;
    garry_byte key[512];
    garry_byte value[512];
    garry_btree_node node;

    pool = garry_pool_create("tm_insert_sorted.db", 8, 4096);
    GARRY_CHECK(pool != NULL);
    root = garry_allocate_leaf(pool);
    GARRY_CHECK(root >= 0);

    ENCODE_KEY(key, "a");
    ENCODE_KEY(value, "1");
    garry_btree_insert(pool, &root, key, 1, value, 1);

    ENCODE_KEY(key, "b");
    ENCODE_KEY(value, "2");
    garry_btree_insert(pool, &root, key, 1, value, 1);

    garry_load_node(pool, root, &node);
    GARRY_CHECK(node.entry_count == 2);
    GARRY_CHECK(memcmp(node.keys[0], "a", 1) == 0);
    GARRY_CHECK(memcmp(node.keys[1], "b", 1) == 0);

    garry_pool_close(pool);
}

static void test_insert_causes_split(void)
{
    garry_buffer_pool *pool;
    garry_i32 root;
    garry_byte key[512];
    garry_byte value[512];
    garry_btree_node root_node;
    garry_btree_node left_node;
    garry_btree_node right_node;

    pool = garry_pool_create("tm_insert_split.db", 8, 4096);
    GARRY_CHECK(pool != NULL);
    root = garry_allocate_leaf(pool);
    GARRY_CHECK(root >= 0);

    ENCODE_KEY(key, "a");
    ENCODE_KEY(value, "1");
    garry_btree_insert(pool, &root, key, 1, value, 1);

    ENCODE_KEY(key, "b");
    ENCODE_KEY(value, "2");
    garry_btree_insert(pool, &root, key, 1, value, 1);

    ENCODE_KEY(key, "c");
    ENCODE_KEY(value, "3");
    garry_btree_insert(pool, &root, key, 1, value, 1);

    garry_load_node(pool, root, &root_node);
    GARRY_CHECK(root_node.kind == GARRY_NODE_INTERNAL);
    GARRY_CHECK(root_node.entry_count == 1);

    garry_load_node(pool, root_node.children[0], &left_node);
    garry_load_node(pool, root_node.children[1], &right_node);
    GARRY_CHECK(left_node.entry_count + right_node.entry_count == 3);

    garry_pool_close(pool);
}

static void test_delete_from_leaf(void)
{
    garry_buffer_pool *pool;
    garry_i32 root;
    garry_byte key[512];
    garry_byte value[512];
    garry_btree_node node;

    pool = garry_pool_create("tm_delete_leaf.db", 8, 4096);
    GARRY_CHECK(pool != NULL);
    root = garry_allocate_leaf(pool);
    GARRY_CHECK(root >= 0);

    ENCODE_KEY(key, "a");
    ENCODE_KEY(value, "1");
    garry_btree_insert(pool, &root, key, 1, value, 1);
    ENCODE_KEY(key, "b");
    ENCODE_KEY(value, "2");
    garry_btree_insert(pool, &root, key, 1, value, 1);

    ENCODE_KEY(key, "b");
    garry_btree_delete(pool, &root, key, 1);

    garry_load_node(pool, root, &node);
    GARRY_CHECK(node.entry_count == 1);
    GARRY_CHECK(memcmp(node.keys[0], "a", 1) == 0);

    garry_pool_close(pool);
}

static void test_search_after_insert(void)
{
    garry_buffer_pool *pool;
    garry_i32 root;
    garry_byte key[512];
    garry_byte value[512];
    garry_byte result[512];
    garry_i32 result_len;

    pool = garry_pool_create("tm_search.db", 8, 4096);
    GARRY_CHECK(pool != NULL);
    root = garry_allocate_leaf(pool);
    GARRY_CHECK(root >= 0);

    ENCODE_KEY(key, "hello");
    ENCODE_KEY(value, "world");
    garry_btree_insert(pool, &root, key, 5, value, 5);

    memset(result, 0, sizeof(result));
    result_len = 0;
    GARRY_CHECK(garry_leaf_find_search(pool, root, key, 5, result, &result_len));
    GARRY_CHECK(result_len == 5);
    GARRY_CHECK(memcmp(result, "world", 5) == 0);

    garry_pool_close(pool);
}

static void test_delete_with_split_rebalance(void)
{
    garry_buffer_pool *pool;
    garry_i32 root;
    garry_byte key[512];
    garry_byte value[512];
    garry_byte result[512];
    garry_i32 result_len;

    pool = garry_pool_create("tm_delete_rebal.db", 8, 4096);
    GARRY_CHECK(pool != NULL);
    root = garry_allocate_leaf(pool);
    GARRY_CHECK(root >= 0);

    ENCODE_KEY(key, "a");
    ENCODE_KEY(value, "1");
    garry_btree_insert(pool, &root, key, 1, value, 1);
    ENCODE_KEY(key, "b");
    ENCODE_KEY(value, "2");
    garry_btree_insert(pool, &root, key, 1, value, 1);
    ENCODE_KEY(key, "c");
    ENCODE_KEY(value, "3");
    garry_btree_insert(pool, &root, key, 1, value, 1);
    ENCODE_KEY(key, "d");
    ENCODE_KEY(value, "4");
    garry_btree_insert(pool, &root, key, 1, value, 1);
    ENCODE_KEY(key, "e");
    ENCODE_KEY(value, "5");
    garry_btree_insert(pool, &root, key, 1, value, 1);

    ENCODE_KEY(key, "a");
    garry_btree_delete(pool, &root, key, 1);
    ENCODE_KEY(key, "b");
    garry_btree_delete(pool, &root, key, 1);

    ENCODE_KEY(key, "c");
    GARRY_CHECK(garry_leaf_find_search(pool, root, key, 1, result, &result_len));
    GARRY_CHECK(result_len == 1);
    GARRY_CHECK(memcmp(result, "3", 1) == 0);

    garry_pool_close(pool);
}

int main(void)
{
    test_insert_single();
    test_insert_sorted();
    test_insert_causes_split();
    test_delete_from_leaf();
    test_search_after_insert();
    test_delete_with_split_rebalance();
    if (garry_test_failures == 0)
        printf("test_btree_modify: ALL PASSED\n");
    return garry_test_failures;
}
