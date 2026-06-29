/**
 * @file keysplit.h
 * @brief String-to-key splitting and reassembly for the Garry storage engine.
 *
 * Converts human-readable delimiter-separated path strings (e.g.
 * "users/matthew/articles/A04") into the internal length-prefixed key
 * format, and back again.
 *
 * Intent: Provide a natural, path-like key syntax for application code
 * while preserving the efficient binary encoding internally.
 * Rationale: Callers think in terms of hierarchical paths; splitting
 * and joining bridges that mental model to the byte-oriented storage.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_KEYSPLIT_H
#define GARRY_KEYSPLIT_H

#include "garry/export.h"
#include "garry/types.h"

/**
 * @brief Split a delimiter-separated string into a length-prefixed key.
 * @param str        Input string (null-terminated).
 * @param delimiter  Character that separates parts (e.g. '/').
 * @param out        Output buffer for the encoded key.
 * @return Byte length of the encoded key, or 0 on error.
 *
 * Example: @c garry_key_split("users/matthew/articles", '/', buf)
 * produces a 3-part key of 20 bytes. The delimiter itself is not stored.
 */
GARRY_API garry_i32 garry_key_split(const char *str, char delimiter, garry_byte_array out);

/**
 * @brief Decode a length-prefixed key back to a delimiter-separated string.
 * @param key       Pointer to the encoded key bytes.
 * @param klen      Length of @p key in bytes.
 * @param delimiter  Character to insert between parts.
 * @param out       Output buffer for the reconstructed string.
 * @param out_size  Capacity of @p out in bytes.
 * @return Number of segments decoded, or 0 if @p out_size is too small.
 */
GARRY_API garry_i32 garry_key_unsplit(const garry_byte *key, garry_i32 klen, char delimiter,
                                      char *out, garry_i32 out_size);

/**
 * @brief Make a single-part key from a C string.
 * @param str  Input string (null-terminated).
 * @param out  Output buffer for the encoded key.
 * @return 1 on success, 0 on error.
 *
 * Equivalent to @c garry_key_split(str, '\\0', out) for single keys.
 */
GARRY_API garry_i32 garry_make_key(const char *str, garry_byte_array out);

/**
 * @brief Make a key from an array of string parts.
 * @param parts   Array of null-terminated C strings.
 * @param count   Number of parts in the array.
 * @param out     Output buffer for the encoded key.
 * @return Byte length of the encoded key, or 0 on error.
 *
 * General-purpose variant when the number of parts is not known at
 * compile time.
 */
GARRY_API garry_i32 garry_make_key_parts(const char **parts, garry_i32 count, garry_byte_array out);

#endif /* GARRY_KEYSPLIT_H */
