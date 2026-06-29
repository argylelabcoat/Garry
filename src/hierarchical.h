/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file hierarchical.h
 * @brief Hierarchical key subscript extraction and manipulation.
 */

#ifndef GARRY_HIERARCHICAL_H
#define GARRY_HIERARCHICAL_H

#include "storage_types.h"
#include "key_encoding.h"

/**
 * @brief Count the number of subscripts in a composite key.
 *
 * A composite key has length-prefixed subscripts: [len1][sub1][len2][sub2]...
 * This function counts how many subscripts are present.
 *
 * @param key   Composite key bytes
 * @param klen  Key length in bytes
 * @return Number of subscripts in the key
 */
garry_i32 garry_subscript_count(const garry_byte *key, garry_i32 klen);

/**
 * @brief Get the byte offset of a specific subscript within a composite key.
 *
 * Walks through the length-prefixed subscripts to find the start
 * position of the requested subscript index.
 *
 * @param key   Composite key bytes
 * @param klen  Key length in bytes
 * @param idx   Subscript index (0-based)
 * @return Byte offset of the subscript, or -1 if idx is out of range
 */
garry_i32 garry_get_subscript_offset(const garry_byte *key, garry_i32 klen, garry_i32 idx);

/**
 * @brief Check if a specific subscript equals a given string.
 *
 * Extracts the subscript at the given index and compares it to expected.
 *
 * @param key      Composite key bytes
 * @param klen     Key length in bytes
 * @param idx      Subscript index to check
 * @param expected Expected string value (null-terminated)
 * @return GARRY_TRUE if subscript matches expected
 */
garry_bool garry_subscript_equal(const garry_byte *key, garry_i32 klen, garry_i32 idx,
                                 const char *expected);

/**
 * @brief Extract the prefix of a composite key (first n subscripts).
 *
 * Writes the first n subscripts back-to-back into the output buffer.
 * Returns the total length of the prefix, or -1 if n exceeds subscript count.
 *
 * @param key       Composite key bytes
 * @param klen      Key length in bytes
 * @param nsubs     Number of subscripts to include in prefix
 * @param out       Output buffer for the prefix
 * @param out_size  Size of output buffer in bytes
 * @return Total length of prefix, or -1 on error
 */
garry_i32 garry_key_prefix(const garry_byte *key, garry_i32 klen, garry_i32 nsubs,
                           garry_byte *out, garry_i32 out_size);

/**
 * @brief Extract a single subscript from a composite key.
 *
 * Copies the subscript bytes at the given index into the output buffer.
 *
 * @param key      Composite key bytes
 * @param klen     Key length in bytes
 * @param idx      Subscript index to extract
 * @param sub      Output buffer for subscript bytes
 * @param sub_len  Output: length of extracted subscript
 */
void garry_extract_subscript(const garry_byte *key, garry_i32 klen, garry_i32 idx,
                             garry_byte *sub, garry_i32 *sub_len);

/**
 * @brief Concatenate a parent key prefix with a child subscript.
 *
 * Appends the child subscript to the parent prefix, creating a full
 * composite key. Returns the total length, or -1 if output buffer is too small.
 *
 * @param parent      Parent key prefix bytes
 * @param parent_len  Parent key prefix length
 * @param sub         Child subscript bytes to append
 * @param sub_len     Child subscript length
 * @param out         Output buffer for concatenated key
 * @param out_size    Size of output buffer in bytes
 * @return Total length of concatenated key, or -1 on error
 */
garry_i32 garry_concat_prefix(const garry_byte *parent, garry_i32 parent_len,
                               const garry_byte *sub, garry_i32 sub_len,
                               garry_byte *out, garry_i32 out_size);

#endif /* GARRY_HIERARCHICAL_H */
