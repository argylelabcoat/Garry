/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file btree_compression.h
 * @brief Key prefix compression utilities for the B-tree.
 */

#ifndef GARRY_BTREE_COMPRESSION_H
#define GARRY_BTREE_COMPRESSION_H

#include "garry/types.h"
#include "storage_types.h"

/**
 * @brief Calculate the length of the common prefix between two byte arrays.
 *
 * Compares bytes from the start of both arrays and returns how many
 * consecutive bytes match. Used for B-tree key prefix compression.
 *
 * @param a     First byte array
 * @param alen  Length of first array
 * @param b     Second byte array
 * @param blen  Length of second array
 * @return Number of bytes in common prefix
 */
garry_i32 garry_common_prefix_length(const garry_byte* a, garry_i32 alen,
                                     const garry_byte* b, garry_i32 blen);

/**
 * @brief Compute the minimum separator key between two adjacent keys.
 *
 * Finds the shortest key that falls between left and right in lexicographic
 * order. Used during B-tree splits to determine the split key.
 *
 * @param result  Output: minimum separator key
 * @param left    Left key bytes
 * @param llen    Left key length
 * @param right   Right key bytes
 * @param rlen    Right key length
 */
void garry_minimum_separator(garry_byte_array result,
                             const garry_byte* left, garry_i32 llen,
                             const garry_byte* right, garry_i32 rlen);

/**
 * @brief Compress a key by removing its common prefix.
 *
 * Stores only the suffix of the key after the shared prefix length.
 * Used for prefix compression in B-tree internal nodes.
 *
 * @param result     Output: compressed key (suffix only)
 * @param full_key   Full uncompressed key
 * @param klen       Full key length in bytes
 * @param prefix_len Number of prefix bytes to strip
 */
void garry_compress_key(garry_byte_array result,
                        const garry_byte* full_key, garry_i32 klen,
                        garry_i32 prefix_len);

/**
 * @brief Decompress a prefix-compressed key.
 *
 * Reconstructs the full key by prepending the previous key's prefix.
 * The compressed form is the suffix after the common prefix.
 *
 * @param result    Output: decompressed full key
 * @param compressed  Compressed key (suffix)
 * @param clen      Compressed key length
 * @param prev_key  Previous key (provides the prefix)
 * @param prev_len  Previous key length
 */
void garry_decompress_key(garry_byte_array result,
                          const garry_byte* compressed, garry_i32 clen,
                          const garry_byte* prev_key, garry_i32 prev_len);

/**
 * @brief Check if two byte arrays are equal.
 *
 * Compares lengths first, then does a byte-by-byte comparison.
 *
 * @param a     First byte array
 * @param alen  Length of first array
 * @param b     Second byte array
 * @param blen  Length of second array
 * @return GARRY_TRUE if arrays are identical
 */
garry_bool garry_byte_equal(const garry_byte* a, garry_i32 alen,
                            const garry_byte* b, garry_i32 blen);

#endif /* GARRY_BTREE_COMPRESSION_H */
