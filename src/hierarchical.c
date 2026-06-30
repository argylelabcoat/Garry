/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file hierarchical.c
 * @brief Hierarchical key space navigation via subscript extraction.
 *
 * Keys in the hierarchical namespace are encoded as sequences of
 * length-prefixed subscripts. This module extracts individual
 * subscripts, counts them, compares specific positions, and builds
 * parent prefixes for scope queries.
 */

#include "hierarchical.h"
#include <string.h>

/**
 * @brief Count the number of subscripts in a hierarchical key.
 *
 * Walks the length-prefixed subscripts and counts them.
 *
 * @param key   Encoded hierarchical key.
 * @param klen  Key length in bytes.
 * @return Number of subscripts in the key.
 */
garry_i32 garry_subscript_count(const garry_byte *key, garry_i32 klen)
{
    garry_i32 count;
    garry_i32 offset;
    garry_i32 plen;
    garry_i32 hdr_size;
    count = 0;
    offset = 0;
    while (offset < klen)
    {
        plen = garry_decode_length_prefix(key, offset);
        hdr_size = garry_length_prefix_size(plen);
        offset = offset + hdr_size + plen;
        count = count + 1;
    }
    return count;
}

/**
 * @brief Get the byte offset of the Nth subscript in a hierarchical key.
 *
 * Skips the first @p idx subscripts and returns the byte offset
 * where the idx-th subscript begins.
 *
 * @param key   Encoded hierarchical key.
 * @param klen  Key length in bytes.
 * @param idx   Subscript index (0-based) to locate.
 * @return Byte offset of the subscript, or -1 if out of bounds.
 */
garry_i32 garry_get_subscript_offset(const garry_byte *key, garry_i32 klen, garry_i32 idx)
{
    garry_i32 offset;
    garry_i32 i;
    garry_i32 plen;
    garry_i32 hdr_size;
    offset = 0;
    for (i = 0; i < idx; i++)
    {
        if (offset >= klen)
            return -1;
        plen = garry_decode_length_prefix(key, offset);
        hdr_size = garry_length_prefix_size(plen);
        offset = offset + hdr_size + plen;
        if (offset > klen)
            return -1;
    }
    return offset;
}

/**
 * @brief Check if a specific subscript equals an expected string.
 *
 * Extracts the @p idx-th subscript from the key and compares it
 * byte-for-byte against @p expected.
 *
 * @param key      Encoded hierarchical key.
 * @param klen     Key length in bytes.
 * @param idx      Subscript index (0-based) to compare.
 * @param expected Expected string value to match.
 * @return 1 if the subscript matches, 0 otherwise.
 */
garry_bool garry_subscript_equal(const garry_byte *key, garry_i32 klen, garry_i32 idx,
                                 const char *expected)
{
    garry_i32 offset;
    garry_i32 i;
    garry_i32 plen;
    garry_i32 hdr_size;
    garry_i32 j;
    garry_i32 exp_len;
    offset = 0;
    for (i = 0; i < idx; i++)
    {
        if (offset >= klen)
            return 0;
        plen = garry_decode_length_prefix(key, offset);
        hdr_size = garry_length_prefix_size(plen);
        offset = offset + hdr_size + plen;
        if (offset > klen)
            return 0;
    }
    if (offset >= klen)
        return 0;
    plen = garry_decode_length_prefix(key, offset);
    hdr_size = garry_length_prefix_size(plen);
    if (offset + hdr_size + plen > klen)
        return 0;
    offset = offset + hdr_size;
    exp_len = (garry_i32)strlen(expected);
    if (plen != exp_len)
    {
        return 0;
    }
    for (j = 0; j < plen; j++)
    {
        if ((garry_i32)key[offset + j] != (garry_i32)(unsigned char)expected[j])
        {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Extract the prefix of a hierarchical key containing the first N subscripts.
 *
 * Copies the bytes corresponding to the first @p nsubs subscripts
 * into the output buffer.
 *
 * @param key      Encoded hierarchical key.
 * @param klen     Key length in bytes.
 * @param nsubs    Number of subscripts to include in the prefix.
 * @param out      Output buffer for the prefix.
 * @param out_size Size of the output buffer.
 * @return Length of the prefix copied, or -1 if the output buffer is too small.
 */
garry_i32 garry_key_prefix(const garry_byte *key, garry_i32 klen, garry_i32 nsubs, garry_byte *out,
                           garry_i32 out_size)
{
    garry_i32 plen;
    garry_i32 i;
    plen = garry_get_subscript_offset(key, klen, nsubs);
    if (plen > out_size)
        return -1;
    memset(out, 0, (size_t)out_size);
    for (i = 0; i < plen; i++)
    {
        out[i] = key[i];
    }
    return plen;
}

/**
 * @brief Extract a single subscript from a hierarchical key by index.
 *
 * Decodes the length-prefixed subscript at position @p idx and
 * copies its raw bytes into @p sub.
 *
 * @param key     Encoded hierarchical key.
 * @param klen    Key length in bytes.
 * @param idx     Subscript index (0-based) to extract.
 * @param sub     Output buffer for the subscript data.
 * @param sub_len Output parameter for the subscript length.
 */
void garry_extract_subscript(const garry_byte *key, garry_i32 klen, garry_i32 idx, garry_byte *sub,
                             garry_i32 *sub_len)
{
    garry_i32 offset;
    garry_i32 plen;
    garry_i32 hdr_size;
    garry_i32 src_off;
    garry_i32 j;
    offset = garry_get_subscript_offset(key, klen, idx);
    plen = garry_decode_length_prefix(key, offset);
    hdr_size = garry_length_prefix_size(plen);
    src_off = offset + hdr_size;
    *sub_len = plen;
    for (j = 0; j < plen; j++)
    {
        sub[j] = key[src_off + j];
    }
}

/**
 * @brief Concatenate a parent prefix with a new subscript.
 *
 * Appends a length-prefixed subscript to an existing parent key prefix,
 * producing a new hierarchical key.
 *
 * @param parent     Parent key prefix.
 * @param parent_len Length of @p parent.
 * @param sub        Subscript to append.
 * @param sub_len    Length of @p sub.
 * @param out        Output buffer for the concatenated key.
 * @param out_size   Size of the output buffer.
 * @return Total length of the concatenated key, or -1 if output buffer is too small.
 */
garry_i32 garry_concat_prefix(const garry_byte *parent, garry_i32 parent_len, const garry_byte *sub,
                              garry_i32 sub_len, garry_byte *out, garry_i32 out_size)
{
    garry_i32 offset;
    garry_i32 total;
    garry_i32 i;
    total = parent_len + garry_length_prefix_size(sub_len) + sub_len;
    if (total > out_size)
        return -1;
    memset(out, 0, (size_t)out_size);
    for (i = 0; i < parent_len; i++)
    {
        out[i] = parent[i];
    }
    offset = garry_encode_length_prefix(out, parent_len, sub_len);
    for (i = 0; i < sub_len; i++)
    {
        out[offset + i] = sub[i];
    }
    return total;
}
