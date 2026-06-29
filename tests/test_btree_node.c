/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "btree_node.h"
#include "test_helpers.h"

int main(void)
{
    garry_btree_node node;
    garry_byte key[512], value[512];
    garry_byte meta_buf[256];
    garry_i32 meta_len;
    garry_btree_node decoded;
    garry_bool ok;

    /* Test LEAF_INSERT */
    memset(&node, 0, sizeof(node));
    node.kind = GARRY_NODE_LEAF;
    node.entry_count = 0;
    node.next_page = -1;
    node.prev_page = -1;

    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));
    key[0] = 'a';
    key[1] = 'b';
    value[0] = 'x';
    value[1] = 'y';
    ok = garry_leaf_insert(&node, key, 2, value, 2, 0);
    GARRY_CHECK(ok == 1);
    GARRY_CHECK(node.entry_count == 1);
    GARRY_CHECK(node.keys[0][0] == 'a');
    GARRY_CHECK(node.keys[0][1] == 'b');
    GARRY_CHECK(node.values[0][0] == 'x');
    GARRY_CHECK(node.value_lens[0] == 2);

    /* Insert at end */
    key[0] = 'c';
    value[0] = 'z';
    ok = garry_leaf_insert(&node, key, 1, value, 1, 1);
    GARRY_CHECK(ok == 1);
    GARRY_CHECK(node.entry_count == 2);
    GARRY_CHECK(node.keys[1][0] == 'c');

    /* Insert in middle (shift) */
    key[0] = 'b';
    value[0] = 'w';
    ok = garry_leaf_insert(&node, key, 1, value, 1, 1);
    GARRY_CHECK(ok == 1);
    GARRY_CHECK(node.entry_count == 3);
    GARRY_CHECK(node.keys[1][0] == 'b');
    GARRY_CHECK(node.keys[2][0] == 'c');

    /* Overflow should fail */
    node.entry_count = GARRY_MAX_KEYS_PER_NODE;
    ok = garry_leaf_insert(&node, key, 1, value, 1, 0);
    GARRY_CHECK(ok == 0);

    /* Test ENCODE_NODE_META / DECODE_NODE_META roundtrip */
    memset(&node, 0, sizeof(node));
    node.kind = GARRY_NODE_LEAF;
    node.entry_count = 0;
    node.next_page = 42;
    node.prev_page = -1;
    meta_len = garry_encode_node_meta(&node, meta_buf);
    GARRY_CHECK(meta_len > 0);

    memset(&decoded, 0, sizeof(decoded));
    decoded.kind = GARRY_NODE_LEAF;
    garry_decode_node_meta(meta_buf, meta_len, &decoded);
    GARRY_CHECK(decoded.next_page == 42);
    GARRY_CHECK(decoded.prev_page == -1);

    /* Internal node with children */
    memset(&node, 0, sizeof(node));
    node.kind = GARRY_NODE_INTERNAL;
    node.entry_count = 2;
    node.next_page = -1;
    node.prev_page = -1;
    node.children[0] = 10;
    node.children[1] = 20;
    node.children[2] = 30;
    meta_len = garry_encode_node_meta(&node, meta_buf);
    GARRY_CHECK(meta_len > 0);

    memset(&decoded, 0, sizeof(decoded));
    decoded.kind = GARRY_NODE_INTERNAL;
    garry_decode_node_meta(meta_buf, meta_len, &decoded);
    GARRY_CHECK(decoded.children[0] == 10);
    GARRY_CHECK(decoded.children[1] == 20);
    GARRY_CHECK(decoded.children[2] == 30);

    return garry_test_failures;
}
