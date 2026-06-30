/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file key_encoding.c
 * @brief Length-prefixed tuple key encoding and comparison.
 *
 * Encodes compound keys as length-prefixed components, enabling
 * byte-lexicographic ordering that preserves the logical component
 * ordering. Supports 2-, 3-, and 4-component keys, plus integer
 * subscript encoding for hierarchical key spaces.
 */

#include "key_encoding.h"
#include <string.h>

/**
 * @brief Calculate the total encoded length of a key tuple.
 *
 * Sums the length-prefixed encoded size of each component.
 *
 * @param t  Key tuple to measure.
 * @return Total encoded byte count.
 */
garry_i32 garry_tuple_length(garry_key_tuple *t)
{
    garry_i32 total = 0, i;
    for (i = 0; i < t->count; i++)
    {
        total += garry_length_prefix_size(t->counts[i]) + t->counts[i];
    }
    return total;
}

/**
 * @brief Create a 2-component key tuple from C strings.
 *
 * @param p1  First component string.
 * @param p2  Second component string.
 * @return Key tuple with 2 parts.
 */
garry_key_tuple garry_make_key_2(const char *p1, const char *p2)
{
    garry_key_tuple t;
    t.count = 2;
    t.parts[0] = (const garry_byte *)p1;
    t.parts[1] = (const garry_byte *)p2;
    t.counts[0] = (garry_i32)strlen(p1);
    t.counts[1] = (garry_i32)strlen(p2);
    return t;
}

/**
 * @brief Create a 3-component key tuple from C strings.
 *
 * @param p1  First component string.
 * @param p2  Second component string.
 * @param p3  Third component string.
 * @return Key tuple with 3 parts.
 */
garry_key_tuple garry_make_key_3(const char *p1, const char *p2, const char *p3)
{
    garry_key_tuple t;
    t.count = 3;
    t.parts[0] = (const garry_byte *)p1;
    t.parts[1] = (const garry_byte *)p2;
    t.parts[2] = (const garry_byte *)p3;
    t.counts[0] = (garry_i32)strlen(p1);
    t.counts[1] = (garry_i32)strlen(p2);
    t.counts[2] = (garry_i32)strlen(p3);
    return t;
}

/**
 * @brief Create a 4-component key tuple from C strings.
 *
 * @param p1  First component string.
 * @param p2  Second component string.
 * @param p3  Third component string.
 * @param p4  Fourth component string.
 * @return Key tuple with 4 parts.
 */
garry_key_tuple garry_make_key_4(const char *p1, const char *p2, const char *p3, const char *p4)
{
    garry_key_tuple t;
    t.count = 4;
    t.parts[0] = (const garry_byte *)p1;
    t.parts[1] = (const garry_byte *)p2;
    t.parts[2] = (const garry_byte *)p3;
    t.parts[3] = (const garry_byte *)p4;
    t.counts[0] = (garry_i32)strlen(p1);
    t.counts[1] = (garry_i32)strlen(p2);
    t.counts[2] = (garry_i32)strlen(p3);
    t.counts[3] = (garry_i32)strlen(p4);
    return t;
}

/**
 * @brief Encode a length prefix at a given offset in a byte array.
 *
 * Uses 1-byte inline encoding for lengths below GARRY_LEN_PREFIX_INLINE_MAX,
 * and 3-byte long encoding otherwise.
 *
 * @param result  Output byte array.
 * @param offset  Byte offset to write the prefix.
 * @param plen    Payload length to encode.
 * @return Byte offset immediately after the written prefix (offset + 1 or + 3).
 */
garry_i32 garry_encode_length_prefix(garry_byte_array result, garry_i32 offset, garry_i32 plen)
{
    if (plen < GARRY_LEN_PREFIX_INLINE_MAX)
    {
        result[offset] = (garry_byte)plen;
        return offset + 1;
    }
    else
    {
        result[offset] = (garry_byte)GARRY_LEN_PREFIX_LONG_MARKER;
        result[offset + 1] = (garry_byte)((plen / 256) % 256);
        result[offset + 2] = (garry_byte)(plen % 256);
        return offset + 3;
    }
}

/**
 * @brief Decode a length prefix from a byte array at a given offset.
 *
 * @param key     Byte array containing the encoded prefix.
 * @param offset  Byte offset of the prefix.
 * @return Decoded payload length.
 */
garry_i32 garry_decode_length_prefix(const garry_byte *key, garry_i32 offset)
{
    garry_i32 v0, v1, v2;
    v0 = (garry_i32)key[offset];
    if (v0 != GARRY_LEN_PREFIX_LONG_MARKER)
    {
        return v0;
    }
    v1 = (garry_i32)key[offset + 1];
    v2 = (garry_i32)key[offset + 2];
    return v1 * 256 + v2;
}

/**
 * @brief Return the number of bytes a length prefix occupies.
 *
 * @param plen  Payload length.
 * @return 1 for inline encoding, 3 for long encoding.
 */
garry_i32 garry_length_prefix_size(garry_i32 plen)
{
    return (plen < GARRY_LEN_PREFIX_INLINE_MAX) ? 1 : 3;
}

/**
 * @brief Encode a key tuple into a byte array.
 *
 * Writes each component as a length-prefixed byte sequence.
 *
 * @param t   Key tuple to encode.
 * @param out Output byte array.
 */
void garry_encode_key_tuple(const garry_key_tuple *t, garry_byte_array out)
{
    garry_i32 offset, i, plen, j;
    memset(out, 0, sizeof(garry_byte_array));
    offset = 0;
    for (i = 0; i < t->count; i++)
    {
        plen = t->counts[i];
        offset = garry_encode_length_prefix(out, offset, plen);
        for (j = 0; j < plen; j++)
        {
            out[offset] = t->parts[i][j];
            offset = offset + 1;
        }
    }
}

/**
 * @brief Decode a byte array into a key tuple.
 *
 * Parses length-prefixed components from the encoded data, setting
 * pointer parts to point directly into the encoded buffer.
 *
 * @param encoded  Encoded key bytes.
 * @param elen     Length of @p encoded.
 * @return Decoded key tuple with component pointers and counts.
 */
garry_key_tuple garry_decode_key_tuple(const garry_byte *encoded, garry_i32 elen)
{
    garry_key_tuple result;
    garry_i32 offset, idx, plen, hdr_size;
    result.count = 0;
    offset = 0;
    idx = 0;
    while (offset < elen && idx < GARRY_MAX_SUBSCRIPTS)
    {
        plen = garry_decode_length_prefix(encoded, offset);
        hdr_size = (encoded[offset] == GARRY_LEN_PREFIX_LONG_MARKER) ? 3 : 1;
        result.parts[idx] = encoded + offset + hdr_size;
        result.counts[idx] = plen;
        result.count = result.count + 1;
        offset = offset + hdr_size + plen;
        idx = idx + 1;
    }
    return result;
}

/**
 * @brief Check whether a key starts with a given prefix.
 *
 * @param key     Key to check.
 * @param klen    Length of @p key.
 * @param prefix  Prefix to match.
 * @param plen    Length of @p prefix.
 * @return 1 if @p key starts with @p prefix, 0 otherwise.
 */
garry_bool garry_has_prefix(const garry_byte *key, garry_i32 klen, const garry_byte *prefix,
                            garry_i32 plen)
{
    garry_i32 i;
    if (klen < plen)
    {
        return 0;
    }
    for (i = 0; i < plen; i++)
    {
        if (key[i] != prefix[i])
        {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Zero-fill a byte array.
 *
 * @param out  Byte array to clear.
 */
void garry_empty_byte_array(garry_byte_array out)
{
    memset(out, 0, sizeof(garry_byte_array));
}

/**
 * @brief Encode an integer as a length-prefixed subscript.
 *
 * Writes a type marker byte (0x02) followed by 8 bytes of the
 * integer in little-endian order.
 *
 * @param n   Integer value to encode.
 * @param out Output byte array.
 */
void garry_encode_integer_subscript(garry_i32 n, garry_byte_array out)
{
    garry_i32 i;
    garry_u32 val;
    memset(out, 0, sizeof(garry_byte_array));
    out[0] = (garry_byte)2;
    val = (garry_u32)n;
    for (i = 1; i <= 8; i++)
    {
        out[i] = (garry_byte)(val & 0xFF);
        val >>= 8;
    }
}

/**
 * @brief Decode an integer subscript from a byte array.
 *
 * Reads 8 bytes in little-endian order starting at @p offset + 1
 * (skipping the type marker).
 *
 * @param encoded  Encoded byte array.
 * @param offset   Byte offset of the type marker.
 * @return Decoded integer value.
 */
garry_i32 garry_decode_integer_subscript(const garry_byte *encoded, garry_i32 offset)
{
    garry_i32 result = 0;
    garry_i32 i;
    for (i = 8; i >= 1; i--)
    {
        result = result * 256 + (garry_i32)(garry_u8)encoded[offset + i];
    }
    return result;
}

/**
 * @brief Compare two byte arrays lexicographically.
 *
 * Compares up to the shorter of the two lengths, then considers
 * length differences if all compared bytes are equal.
 *
 * @param a     First byte array.
 * @param alen  Length of @p a.
 * @param b     Second byte array.
 * @param blen  Length of @p b.
 * @return -1 if a < b, 1 if a > b, 0 if equal.
 */
garry_i32 garry_byte_compare(const garry_byte *a, garry_i32 alen, const garry_byte *b,
                             garry_i32 blen)
{
    garry_i32 i, limit;
    limit = (alen < blen) ? alen : blen;
    for (i = 0; i < limit; i++)
    {
        if ((garry_u8)a[i] < (garry_u8)b[i])
            return -1;
        if ((garry_u8)a[i] > (garry_u8)b[i])
            return 1;
    }
    if (alen < blen)
        return -1;
    if (alen > blen)
        return 1;
    return 0;
}
