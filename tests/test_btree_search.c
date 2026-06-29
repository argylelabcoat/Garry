/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "btree_search.h"
#include "btree_node.h"
#include "test_helpers.h"

int main(void)
{
    garry_btree_node node;
    garry_byte key[512];
    garry_i32 idx;

    /* LEAF_FIND: empty leaf */
    memset(&node, 0, sizeof(node));
    node.kind = GARRY_LEAF_NODE;
    node.entry_count = 0;
    memset(key, 0, sizeof(key));
    key[0] = 'a';
    idx = garry_leaf_find(&node, key, 1);
    GARRY_CHECK(idx == -1);

    /* LEAF_FIND: insert keys, find them */
    node.entry_count = 3;
    node.keys[0][0] = 'a'; node.key_lens[0] = 1;
    node.keys[1][0] = 'b'; node.key_lens[1] = 1;
    node.keys[2][0] = 'c'; node.key_lens[2] = 1;

    key[0] = 'a';
    idx = garry_leaf_find(&node, key, 1);
    GARRY_CHECK(idx == 0);

    key[0] = 'b';
    idx = garry_leaf_find(&node, key, 1);
    GARRY_CHECK(idx == 1);

    key[0] = 'c';
    idx = garry_leaf_find(&node, key, 1);
    GARRY_CHECK(idx == 2);

    key[0] = 'd';
    idx = garry_leaf_find(&node, key, 1);
    GARRY_CHECK(idx == -1);

    /* INTERNAL_FIND: find child index */
    node.kind = GARRY_INTERNAL_NODE;
    node.entry_count = 3;
    node.keys[0][0] = 'a'; node.key_lens[0] = 1;
    node.keys[1][0] = 'c'; node.key_lens[1] = 1;
    node.keys[2][0] = 'e'; node.key_lens[2] = 1;

    key[0] = 'a';  /* equal to first key -> not < first, check next */
    idx = garry_internal_find(&node, key, 1);
    GARRY_CHECK(idx == 1);

    key[0] = 'b';  /* between a and c -> index 1 */
    idx = garry_internal_find(&node, key, 1);
    GARRY_CHECK(idx == 1);

    key[0] = 'c';  /* equal to second key -> not < second, check next */
    idx = garry_internal_find(&node, key, 1);
    GARRY_CHECK(idx == 2);

    key[0] = 'd';  /* between c and e -> index 2 */
    idx = garry_internal_find(&node, key, 1);
    GARRY_CHECK(idx == 2);

    key[0] = 'f';  /* after all keys -> entry_count */
    idx = garry_internal_find(&node, key, 1);
    GARRY_CHECK(idx == 3);

    /* Multi-byte keys */
    node.kind = GARRY_LEAF_NODE;
    node.entry_count = 2;
    memcpy(node.keys[0], "hello", 5); node.key_lens[0] = 5;
    memcpy(node.keys[1], "world", 5); node.key_lens[1] = 5;

    idx = garry_leaf_find(&node, (const garry_byte*)"hello", 5);
    GARRY_CHECK(idx == 0);
    idx = garry_leaf_find(&node, (const garry_byte*)"world", 5);
    GARRY_CHECK(idx == 1);
    idx = garry_leaf_find(&node, (const garry_byte*)"hell", 4);
    GARRY_CHECK(idx == -1);

    return garry_test_failures;
}
