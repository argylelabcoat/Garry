/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "btree_node.h"
#include "buffer_pool.h"
#include "key_encoding.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>

static int test_allocate_leaf_roundtrip(void)
{
    garry_buffer_pool *pool;
    garry_i32 pid;
    garry_btree_node loaded;

    pool = garry_pool_create("tnp_leaf.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    pid = garry_allocate_leaf(pool);
    GARRY_CHECK(pid >= 0);
    memset(&loaded, 0, sizeof(loaded));
    garry_load_node(pool, pid, &loaded);
    GARRY_CHECK(loaded.kind == GARRY_NODE_LEAF);
    GARRY_CHECK(loaded.entry_count == 0);
    garry_pool_close(pool);
    return 0;
}

static int test_allocate_internal_roundtrip(void)
{
    garry_buffer_pool *pool;
    garry_i32 pid;
    garry_btree_node loaded;

    pool = garry_pool_create("tnp_int.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    pid = garry_allocate_internal(pool);
    GARRY_CHECK(pid >= 0);
    memset(&loaded, 0, sizeof(loaded));
    garry_load_node(pool, pid, &loaded);
    GARRY_CHECK(loaded.kind == GARRY_NODE_INTERNAL);
    GARRY_CHECK(loaded.entry_count == 0);
    garry_pool_close(pool);
    return 0;
}

static int test_store_load_single_entry(void)
{
    garry_buffer_pool *pool;
    garry_i32 pid;
    garry_btree_node node, loaded;
    garry_byte_array key, val;

    pool = garry_pool_create("tnp_store.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    pid = garry_allocate_leaf(pool);
    GARRY_CHECK(pid >= 0);

    memset(&node, 0, sizeof(node));
    node.kind = GARRY_NODE_LEAF;
    node.header.page_type = GARRY_NODE_LEAF;
    node.header.page_level = 0;
    node.next_page = -1;
    node.prev_page = -1;
    node.entry_count = 1;
    ENCODE_KEY(key, "hello");
    ENCODE_KEY(val, "world");
    memcpy(node.keys[0], key, sizeof(garry_byte_array));
    node.key_lens[0] = 5;
    memcpy(node.values[0], val, sizeof(garry_byte_array));
    node.value_lens[0] = 5;

    garry_store_node(pool, pid, &node);

    memset(&loaded, 0, sizeof(loaded));
    garry_load_node(pool, pid, &loaded);
    GARRY_CHECK(loaded.entry_count == 1);
    GARRY_CHECK(loaded.key_lens[0] == 5);
    GARRY_CHECK(memcmp(loaded.keys[0], "hello", 5) == 0);
    GARRY_CHECK(loaded.value_lens[0] == 5);
    GARRY_CHECK(memcmp(loaded.values[0], "world", 5) == 0);

    garry_pool_close(pool);
    return 0;
}

static int test_store_load_multiple_entries(void)
{
    garry_buffer_pool *pool;
    garry_i32 pid;
    garry_btree_node node, loaded;
    garry_byte_array k1, k2, k3, v1, v2, v3;

    pool = garry_pool_create("tnp_multi.db", 4, 4096);
    GARRY_CHECK(pool != NULL);
    pid = garry_allocate_leaf(pool);
    GARRY_CHECK(pid >= 0);

    memset(&node, 0, sizeof(node));
    node.kind = GARRY_NODE_LEAF;
    node.header.page_type = GARRY_NODE_LEAF;
    node.header.page_level = 0;
    node.next_page = -1;
    node.prev_page = -1;
    node.entry_count = 3;
    ENCODE_KEY(k1, "a");
    ENCODE_KEY(v1, "v1");
    ENCODE_KEY(k2, "b");
    ENCODE_KEY(v2, "v2");
    ENCODE_KEY(k3, "c");
    ENCODE_KEY(v3, "v3");
    memcpy(node.keys[0], k1, sizeof(garry_byte_array));
    node.key_lens[0] = 1;
    memcpy(node.values[0], v1, sizeof(garry_byte_array));
    node.value_lens[0] = 2;
    memcpy(node.keys[1], k2, sizeof(garry_byte_array));
    node.key_lens[1] = 1;
    memcpy(node.values[1], v2, sizeof(garry_byte_array));
    node.value_lens[1] = 2;
    memcpy(node.keys[2], k3, sizeof(garry_byte_array));
    node.key_lens[2] = 1;
    memcpy(node.values[2], v3, sizeof(garry_byte_array));
    node.value_lens[2] = 2;

    garry_store_node(pool, pid, &node);

    memset(&loaded, 0, sizeof(loaded));
    garry_load_node(pool, pid, &loaded);
    GARRY_CHECK(loaded.entry_count == 3);
    GARRY_CHECK(memcmp(loaded.keys[0], "a", 1) == 0);
    GARRY_CHECK(memcmp(loaded.keys[1], "b", 1) == 0);
    GARRY_CHECK(memcmp(loaded.keys[2], "c", 1) == 0);
    GARRY_CHECK(memcmp(loaded.values[0], "v1", 2) == 0);
    GARRY_CHECK(memcmp(loaded.values[2], "v3", 2) == 0);

    garry_pool_close(pool);
    return 0;
}

int main(void)
{
    int r = 0;
    r += test_allocate_leaf_roundtrip();
    r += test_allocate_internal_roundtrip();
    r += test_store_load_single_entry();
    r += test_store_load_multiple_entries();
    if (r == 0)
        printf("test_btree_node_pool: ALL PASSED\n");
    return r;
}
