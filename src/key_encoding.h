/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file key_encoding.h
 * @brief Tuple-based key encoding for compound keys.
 */

#ifndef GARRY_KEY_ENCODING_H
#define GARRY_KEY_ENCODING_H

#include "garry/config.h"
#include "garry/types.h"
#include "garry/encoding.h"
#include "storage_types.h"
#include <string.h>

/**
 * @brief Calculate the total encoded length of a key tuple.
 *
 * Sums the length-prefix sizes and part lengths for all parts in the tuple.
 *
 * @param t  Key tuple to measure
 * @return Total encoded length in bytes
 */
garry_i32 garry_tuple_length(garry_key_tuple *t);

/**
 * @brief Create a 2-part compound key from C strings.
 *
 * @param p1  First part (null-terminated C string)
 * @param p2  Second part (null-terminated C string)
 * @return Key tuple with 2 parts
 */
garry_key_tuple garry_make_key_2(const char *p1, const char *p2);

/**
 * @brief Create a 3-part compound key from C strings.
 *
 * @param p1  First part (null-terminated C string)
 * @param p2  Second part (null-terminated C string)
 * @param p3  Third part (null-terminated C string)
 * @return Key tuple with 3 parts
 */
garry_key_tuple garry_make_key_3(const char *p1, const char *p2, const char *p3);

/**
 * @brief Create a 4-part compound key from C strings.
 *
 * @param p1  First part (null-terminated C string)
 * @param p2  Second part (null-terminated C string)
 * @param p3  Third part (null-terminated C string)
 * @param p4  Fourth part (null-terminated C string)
 * @return Key tuple with 4 parts
 */
garry_key_tuple garry_make_key_4(const char *p1, const char *p2, const char *p3, const char *p4);

/**
 * @brief Encode a length prefix at the given offset.
 *
 * Writes a variable-length integer encoding of plen at offset in result.
 * The prefix indicates the length of the following key part.
 *
 * @param result  Output byte array to write into
 * @param offset  Byte offset where prefix starts
 * @param plen    Length value to encode
 * @return Number of bytes written for the prefix
 */
garry_i32 garry_encode_length_prefix(garry_byte_array result, garry_i32 offset, garry_i32 plen);

/**
 * @brief Decode a length prefix from a key at the given offset.
 *
 * Reads a variable-length integer encoding from offset in key.
 *
 * @param key     Byte array containing the encoded prefix
 * @param offset  Byte offset where prefix starts
 * @return Decoded length value
 */
garry_i32 garry_decode_length_prefix(const garry_byte *key, garry_i32 offset);

/**
 * @brief Calculate the number of bytes needed to encode a length value.
 *
 * @param plen  Length value to measure
 * @return Number of bytes needed for variable-length encoding
 */
garry_i32 garry_length_prefix_size(garry_i32 plen);

/**
 * @brief Encode a key tuple into its byte representation.
 *
 * Writes each part with its length prefix into the output buffer.
 * The encoded form is: [prefix1][part1][prefix2][part2]...
 *
 * @param t    Key tuple to encode
 * @param out  Output byte array (must be large enough)
 */
void garry_encode_key_tuple(const garry_key_tuple *t, garry_byte_array out);

/**
 * @brief Decode a byte buffer into a key tuple.
 *
 * Inverse of garry_encode_key_tuple. Parses length prefixes and
 * extracts parts into a key tuple structure.
 *
 * @param encoded  Byte buffer containing encoded tuple
 * @param elen     Length of encoded data in bytes
 * @return Decoded key tuple with parts pointing into encoded buffer
 */
garry_key_tuple garry_decode_key_tuple(const garry_byte *encoded, garry_i32 elen);

/**
 * @brief Check if a key starts with a given prefix.
 *
 * Compares the first plen bytes of key against prefix.
 *
 * @param key     Key bytes to check
 * @param klen    Key length in bytes
 * @param prefix  Prefix bytes to match
 * @param plen    Prefix length in bytes
 * @return GARRY_TRUE if key starts with prefix
 */
garry_bool garry_has_prefix(const garry_byte *key, garry_i32 klen, const garry_byte *prefix,
                            garry_i32 plen);

/**
 * @brief Zero out a byte array.
 *
 * Fills the byte array with zeros. Used to initialize empty keys.
 *
 * @param out  Byte array to zero out
 */
void garry_empty_byte_array(garry_byte_array out);

/**
 * @brief Encode an integer subscript value into a byte array.
 *
 * Writes the integer as a big-endian byte sequence at the start of out.
 *
 * @param n    Integer value to encode
 * @param out  Output byte array
 */
void garry_encode_integer_subscript(garry_i32 n, garry_byte_array out);

/**
 * @brief Decode an integer subscript from a byte buffer.
 *
 * Reads a big-endian integer from the buffer at the given offset.
 *
 * @param encoded  Byte buffer containing encoded integer
 * @param offset   Byte offset where integer starts
 * @return Decoded integer value
 */
garry_i32 garry_decode_integer_subscript(const garry_byte *encoded, garry_i32 offset);

/**
 * @brief Compare two byte arrays lexicographically.
 *
 * @param a     First byte array
 * @param alen  Length of first array
 * @param b     Second byte array
 * @param blen  Length of second array
 * @return Negative if a < b, 0 if equal, positive if a > b
 */
garry_i32 garry_byte_compare(const garry_byte *a, garry_i32 alen, const garry_byte *b,
                             garry_i32 blen);

#endif /* GARRY_KEY_ENCODING_H */
