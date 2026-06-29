/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "btree_node.h"
#include "btree_modify.h"
#include "btree_search.h"
#include "slotted_page.h"
#include "version_chain.h"
#include "buffer_pool.h"
#include "test_helpers.h"
#include <string.h>
#include <stdlib.h>

static void test_btree_single_key_leaf(void)
{
    garry_btree_node node;
    garry_byte key[512], value[512];
    garry_i32 idx;

    printf("test_btree_single_key_leaf\n");
    memset(&node, 0, sizeof(node));
    node.kind = GARRY_NODE_LEAF;
    node.entry_count = 0;
    node.next_page = -1;
    node.prev_page = -1;

    key[0] = 'x';
    value[0] = 'y';
    GARRY_CHECK(garry_leaf_insert(&node, key, 1, value, 1, 0) == 1);
    GARRY_CHECK(node.entry_count == 1);
    GARRY_CHECK(node.keys[0][0] == 'x');
    GARRY_CHECK(node.value_lens[0] == 1);

    idx = garry_leaf_find(&node, key, 1);
    GARRY_CHECK(idx == 0);

    key[0] = 'z';
    idx = garry_leaf_find(&node, key, 1);
    GARRY_CHECK(idx == -1);
}

static void test_btree_empty_node_encode_decode(void)
{
    garry_btree_node node, decoded;
    garry_byte meta_buf[256];
    garry_i32 meta_len;

    printf("test_btree_empty_node_encode_decode\n");
    memset(&node, 0, sizeof(node));
    node.kind = GARRY_NODE_LEAF;
    node.entry_count = 0;
    node.next_page = -1;
    node.prev_page = -1;

    meta_len = garry_encode_node_meta(&node, meta_buf);
    GARRY_CHECK(meta_len > 0);

    memset(&decoded, 0, sizeof(decoded));
    decoded.kind = GARRY_NODE_LEAF;
    garry_decode_node_meta(meta_buf, meta_len, &decoded);
    GARRY_CHECK(decoded.next_page == -1);
    GARRY_CHECK(decoded.prev_page == -1);
    GARRY_CHECK(decoded.entry_count == 0);
}

static void test_btree_full_node_rejects_insert(void)
{
    garry_btree_node node;
    garry_byte key[512], value[512];

    printf("test_btree_full_node_rejects_insert\n");
    memset(&node, 0, sizeof(node));
    node.kind = GARRY_NODE_LEAF;
    node.entry_count = GARRY_MAX_KEYS_PER_NODE;
    node.next_page = -1;
    node.prev_page = -1;

    key[0] = 'z';
    value[0] = '1';
    GARRY_CHECK(garry_leaf_insert(&node, key, 1, value, 1, 0) == 0);
    GARRY_CHECK(node.entry_count == GARRY_MAX_KEYS_PER_NODE);
}

static void test_btree_insert_split_and_search(void)
{
    garry_buffer_pool *pool;
    garry_i32 root;
    garry_byte key[512], value[512], result[512];
    garry_i32 result_len;

    printf("test_btree_insert_split_and_search\n");
    pool = garry_pool_create("tm_edge_split.db", 8, 4096);
    GARRY_CHECK(pool != NULL);
    root = garry_allocate_leaf(pool);
    GARRY_CHECK(root >= 0);

    ENCODE_KEY(key, "d");
    ENCODE_KEY(value, "4");
    garry_btree_insert(pool, &root, key, 1, value, 1);

    ENCODE_KEY(key, "b");
    ENCODE_KEY(value, "2");
    garry_btree_insert(pool, &root, key, 1, value, 1);

    ENCODE_KEY(key, "f");
    ENCODE_KEY(value, "6");
    garry_btree_insert(pool, &root, key, 1, value, 1);

    ENCODE_KEY(key, "a");
    ENCODE_KEY(value, "1");
    garry_btree_insert(pool, &root, key, 1, value, 1);

    ENCODE_KEY(key, "c");
    ENCODE_KEY(value, "3");
    garry_btree_insert(pool, &root, key, 1, value, 1);

    ENCODE_KEY(key, "b");
    memset(result, 0, sizeof(result));
    result_len = 0;
    GARRY_CHECK(garry_leaf_find_search(pool, root, key, 1, result, &result_len));
    GARRY_CHECK(result_len == 1);
    GARRY_CHECK(memcmp(result, "2", 1) == 0);

    ENCODE_KEY(key, "f");
    memset(result, 0, sizeof(result));
    result_len = 0;
    GARRY_CHECK(garry_leaf_find_search(pool, root, key, 1, result, &result_len));
    GARRY_CHECK(result_len == 1);
    GARRY_CHECK(memcmp(result, "6", 1) == 0);

    ENCODE_KEY(key, "z");
    GARRY_CHECK(!garry_leaf_find_search(pool, root, key, 1, result, &result_len));

    garry_pool_close(pool);
}

static void test_slotted_page_mixed_sizes(void)
{
    garry_page_buffer buf;
    garry_byte data[512];
    garry_byte out[512];
    garry_i32 idx, rlen;

    printf("test_slotted_page_mixed_sizes\n");
    garry_page_init(buf, GARRY_NODE_LEAF, 0, 4096);

    data[0] = 'A';
    idx = garry_page_insert(buf, data, 1, 4096);
    GARRY_CHECK(idx == 0);

    memset(data, 'B', 100);
    idx = garry_page_insert(buf, data, 100, 4096);
    GARRY_CHECK(idx == 1);

    memset(data, 'C', 50);
    idx = garry_page_insert(buf, data, 50, 4096);
    GARRY_CHECK(idx == 2);

    rlen = garry_page_get(&buf, 0, out, 4096);
    GARRY_CHECK(rlen == 1);
    GARRY_CHECK(out[0] == 'A');

    rlen = garry_page_get(&buf, 1, out, 4096);
    GARRY_CHECK(rlen == 100);
    GARRY_CHECK(out[0] == 'B');
    GARRY_CHECK(out[99] == 'B');

    rlen = garry_page_get(&buf, 2, out, 4096);
    GARRY_CHECK(rlen == 50);
    GARRY_CHECK(out[0] == 'C');
    GARRY_CHECK(out[49] == 'C');

    GARRY_CHECK(garry_page_record_count(&buf) == 3);
}

static void test_slotted_page_invalid_slot_read(void)
{
    garry_page_buffer buf;
    garry_byte out[512];
    garry_i32 rlen;

    printf("test_slotted_page_invalid_slot_read\n");
    garry_page_init(buf, GARRY_NODE_LEAF, 0, 4096);

    rlen = garry_page_get(&buf, 0, out, 4096);
    GARRY_CHECK(rlen == 0);
}

static void test_version_chain_prune_with_active(void)
{
    garry_page_buffer buf;
    garry_i32 vlen;
    char *result;
    garry_txn_id active[4];

    printf("test_version_chain_prune_with_active\n");
    garry_chain_page_init(buf, 4096);

    garry_chain_page_append(buf, 4096, 1, "old1", 4);
    garry_chain_page_append(buf, 4096, 2, "old2", 4);
    garry_chain_page_append(buf, 4096, 3, "new3", 4);

    active[0] = 2;
    garry_chain_page_prune(NULL, buf, 4096, active, 1);

    active[0] = 10;
    result = garry_chain_page_find_visible(NULL, buf, 4096, 3, active, 1, &vlen);
    GARRY_CHECK(result != NULL);
    GARRY_CHECK(vlen == 4);
    GARRY_CHECK(memcmp(result, "new3", 4) == 0);
    free(result);
}

static void test_version_chain_has_version(void)
{
    garry_page_buffer buf;

    printf("test_version_chain_has_version\n");
    garry_chain_page_init(buf, 4096);

    GARRY_CHECK(garry_chain_page_has_version(buf, 4096, 1) == 0);

    garry_chain_page_append(buf, 4096, 5, "data", 4);
    GARRY_CHECK(garry_chain_page_has_version(buf, 4096, 5) == 1);
    GARRY_CHECK(garry_chain_page_has_version(buf, 4096, 4) == 0);
    GARRY_CHECK(garry_chain_page_has_version(buf, 4096, 6) == 0);
}

static void test_version_chain_multiple_versions_visibility(void)
{
    garry_page_buffer buf;
    garry_i32 vlen;
    char *result;
    garry_txn_id active[4];

    printf("test_version_chain_multiple_versions_visibility\n");
    garry_chain_page_init(buf, 4096);

    garry_chain_page_append(buf, 4096, 1, "v1", 2);
    garry_chain_page_append(buf, 4096, 3, "v3", 2);
    garry_chain_page_append(buf, 4096, 5, "v5", 2);

    active[0] = 10;
    result = garry_chain_page_find_visible(NULL, buf, 4096, 5, active, 1, &vlen);
    GARRY_CHECK(result != NULL);
    GARRY_CHECK(vlen == 2);
    GARRY_CHECK(memcmp(result, "v5", 2) == 0);
    free(result);

    result = garry_chain_page_find_visible(NULL, buf, 4096, 3, active, 1, &vlen);
    GARRY_CHECK(result != NULL);
    GARRY_CHECK(vlen == 2);
    GARRY_CHECK(memcmp(result, "v3", 2) == 0);
    free(result);

    result = garry_chain_page_find_visible(NULL, buf, 4096, 1, active, 1, &vlen);
    GARRY_CHECK(result != NULL);
    GARRY_CHECK(vlen == 2);
    GARRY_CHECK(memcmp(result, "v1", 2) == 0);
    free(result);

    result = garry_chain_page_find_visible(NULL, buf, 4096, 0, active, 1, &vlen);
    GARRY_CHECK(result == NULL);
}

static void test_version_chain_overflow_append(void)
{
    garry_page_buffer buf;
    garry_bool ok;
    garry_i32 inline_cap;

    printf("test_version_chain_overflow_append\n");
    garry_chain_page_init(buf, 4096);

    inline_cap = garry_chain_inline_capacity(4096);
    GARRY_CHECK(inline_cap > 0);

    ok = garry_chain_page_append(buf, 4096, 1, "small", 5);
    GARRY_CHECK(ok == 1);

    ok = garry_chain_page_append_tombstone(buf, 4096, 2);
    GARRY_CHECK(ok == 1);

    GARRY_CHECK(garry_chain_page_has_version(buf, 4096, 1) == 1);
    GARRY_CHECK(garry_chain_page_has_version(buf, 4096, 2) == 1);
}

int main(void)
{
    test_btree_single_key_leaf();
    test_btree_empty_node_encode_decode();
    test_btree_full_node_rejects_insert();
    test_btree_insert_split_and_search();
    test_slotted_page_mixed_sizes();
    test_slotted_page_invalid_slot_read();
    test_version_chain_prune_with_active();
    test_version_chain_has_version();
    test_version_chain_multiple_versions_visibility();
    test_version_chain_overflow_append();

    if (garry_test_failures == 0) printf("test_data_structures: ALL PASSED\n");
    return garry_test_failures;
}
