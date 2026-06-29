/**
 * @file encoding.h
 * @brief Key tuple creation and encoding for the Garry storage engine.
 *
 * Provides functions to build composite keys from string parts, encode
 * them into the internal length-prefixed byte format, and compare
 * encoded keys lexicographically.
 *
 * Intent: Abstract the internal key encoding so callers never need to
 * know the on-disk byte layout of composite keys.
 * Rationale: The length-prefixed encoding is an internal detail that
 * must not leak into application code; these helpers keep the encoding
 * stable even if the internal format changes.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_ENCODING_H
#define GARRY_ENCODING_H

#include "garry/config.h"
#include "garry/export.h"
#include "garry/types.h"

/**
 * @brief A composite key represented as an array of string parts.
 *
 * Used as an intermediate form before encoding to bytes. Build a tuple
 * with @ref garry_make_key_2 / @ref garry_make_key_3 / @ref garry_make_key_4,
 * then call @ref garry_encode_key_tuple to produce the final byte array.
 */
typedef struct
{
    const garry_byte *parts[GARRY_MAX_SUBSCRIPTS]; ///< Pointers to each part.
    garry_i32 counts[GARRY_MAX_SUBSCRIPTS];        ///< Length of each part.
    garry_i32 count;                               ///< Number of active parts.
} garry_key_tuple;

/**
 * @brief Zero-initialize a byte array.
 * @param out  Output buffer (typically a @ref garry_byte_array).
 *
 * Sets every byte in @p out to 0. Useful for clearing a buffer before
 * encoding a key into it.
 */
GARRY_API void garry_empty_byte_array(garry_byte_array out);

/**
 * @brief Build a 2-part composite key from two C strings.
 * @param p1  First part (null-terminated).
 * @param p2  Second part (null-terminated).
 * @return A @ref garry_key_tuple with 2 active parts.
 *
 * Convenience wrapper for the common case of two-part keys
 * (e.g. "users" + "matthew").
 */
GARRY_API garry_key_tuple garry_make_key_2(const char *p1, const char *p2);

/**
 * @brief Build a 3-part composite key from three C strings.
 * @param p1  First part.
 * @param p2  Second part.
 * @param p3  Third part.
 * @return A @ref garry_key_tuple with 3 active parts.
 */
GARRY_API garry_key_tuple garry_make_key_3(const char *p1, const char *p2, const char *p3);

/**
 * @brief Build a 4-part composite key from four C strings.
 * @param p1  First part.
 * @param p2  Second part.
 * @param p3  Third part.
 * @param p4  Fourth part.
 * @return A @ref garry_key_tuple with 4 active parts.
 */
GARRY_API garry_key_tuple garry_make_key_4(const char *p1, const char *p2, const char *p3,
                                           const char *p4);

/**
 * @brief Encode a key tuple into the internal byte format.
 * @param t    Pointer to the key tuple (read-only).
 * @param out  Output buffer (must be at least @ref GARRY_MAX_KEY_SIZE bytes).
 *
 * Writes a length-prefixed encoding of each part into @p out. The
 * resulting byte array can be passed directly to @ref garry_set,
 * @ref garry_get, etc.
 */
GARRY_API void garry_encode_key_tuple(const garry_key_tuple *t, garry_byte_array out);

/**
 * @brief Compare two encoded key buffers lexicographically.
 * @param a     First key buffer.
 * @param alen  Length of @p a in bytes.
 * @param b     Second key buffer.
 * @param blen  Length of @p b in bytes.
 * @return Negative if @p a < @p b, 0 if equal, positive if @p a > @p b.
 *
 * Standard lexicographic comparison suitable for B-tree ordering.
 */
GARRY_API garry_i32 garry_byte_compare(const garry_byte *a, garry_i32 alen, const garry_byte *b,
                                       garry_i32 blen);

#endif /* GARRY_ENCODING_H */
