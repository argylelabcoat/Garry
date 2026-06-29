/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file btree_node.h
 * @brief B-tree node structures and serialization/deserialization.
 *
 * Defines the in-memory representation of B-tree leaf and internal nodes,
 * including CBOR-encoded on-disk format and prefix compression.
 */

#ifndef GARRY_BTREE_NODE_H
#define GARRY_BTREE_NODE_H

#include "garry/config.h"
#include "garry/types.h"
#include "storage_types.h"
#include "storage_core.h"
#include "buffer_pool.h"

typedef struct {
    garry_node_kind kind;
    garry_page_header header;
    garry_byte_array keys[GARRY_MAX_KEYS_PER_NODE];
    garry_i32 key_lens[GARRY_MAX_KEYS_PER_NODE];
    garry_byte_array values[GARRY_MAX_KEYS_PER_NODE];
    garry_i32 value_lens[GARRY_MAX_KEYS_PER_NODE];
    garry_i32 children[GARRY_MAX_KEYS_PER_NODE + 1];
    garry_i32 next_page;
    garry_i32 prev_page;
    garry_i32 entry_count;
} garry_btree_node;

/**
 * @brief Insert a key-value pair into a B-tree leaf node.
 *
 * Shifts existing entries right starting at idx, then writes the new
 * key-value pair at idx. The node must have room (entry_count < MAX_KEYS).
 *
 * @param node  Leaf node to modify (must be a leaf, must have room)
 * @param key   Key bytes to insert
 * @param klen  Key length in bytes
 * @param value Value bytes to insert
 * @param vlen  Value length in bytes
 * @param idx   Position to insert at (0 <= idx <= entry_count)
 * @return GARRY_TRUE on success, GARRY_FALSE if node is full
 */
garry_bool garry_leaf_insert(garry_btree_node* node, const garry_byte* key, garry_i32 klen,
                             const garry_byte* value, garry_i32 vlen, garry_i32 idx);

/**
 * @brief Encode node metadata into a CBOR byte string.
 *
 * Serializes the node's structural metadata (next/prev page IDs,
 * child pointers, entry count) into a compact CBOR representation.
 * Page IDs are biased by GARRY_PAGE_ID_BIAS to avoid negative values.
 *
 * @param node  Node whose metadata to encode
 * @param buf   Output buffer for encoded metadata
 * @return Number of bytes written to buf
 */
garry_i32 garry_encode_node_meta(garry_btree_node* node, garry_byte* buf);

/**
 * @brief Decode node metadata from a CBOR-encoded buffer.
 *
 * The metadata is a CBOR array: [next_page, prev_page, child_0, child_1, ...]
 * where page IDs are stored with GARRY_PAGE_ID_BIAS offset to avoid
 * negative values in CBOR unsigned integer encoding.
 *
 * Algorithm:
 * 1. Parse CBOR array header to get element count
 * 2. Decode next_page and prev_page (elements 0-1)
 * 3. For internal nodes, decode children (elements 2+)
 * 4. Subtract GARRY_PAGE_ID_BIAS from each decoded page ID
 *
 * @param buf   CBOR-encoded buffer to decode
 * @param elen  Length of encoded data in bytes
 * @param node  Node to populate with decoded metadata
 */
void garry_decode_node_meta(const garry_byte* buf, garry_i32 elen, garry_btree_node* node);

/**
 * @brief Allocate a new leaf node from the buffer pool.
 *
 * Obtains a free page from the pool and initializes it as a leaf node
 * with zero entries. The returned page ID can be used with garry_store_node.
 *
 * @param pool  Buffer pool to allocate from
 * @return Page ID of the new leaf node, or -1 on failure
 */
garry_i32 garry_allocate_leaf(garry_buffer_pool *pool);

/**
 * @brief Allocate a new internal node from the buffer pool.
 *
 * Obtains a free page from the pool and initializes it as an internal node
 * with zero entries. Used during B-tree splits.
 *
 * @param pool  Buffer pool to allocate from
 * @return Page ID of the new internal node, or -1 on failure
 */
garry_i32 garry_allocate_internal(garry_buffer_pool *pool);

/**
 * @brief Load a B-tree node from the buffer pool into memory.
 *
 * Reads the page from the pool and deserializes its header and
 * metadata into the in-memory node structure.
 *
 * @param pool  Buffer pool containing the page
 * @param pid   Page ID to load
 * @param node  Output: populated node structure
 */
void garry_load_node(garry_buffer_pool *pool, garry_i32 pid, garry_btree_node *node);

/**
 * @brief Store an in-memory node back to the buffer pool.
 *
 * Serializes the node's header, metadata, and entries into the
 * page buffer and marks it dirty for eventual flush to disk.
 *
 * @param pool  Buffer pool containing the page
 * @param pid   Page ID to store to
 * @param node  Node structure to serialize
 */
void garry_store_node(garry_buffer_pool *pool, garry_i32 pid, garry_btree_node *node);

#endif /* GARRY_BTREE_NODE_H */
