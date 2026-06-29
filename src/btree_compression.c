/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file btree_compression.c
 * @brief Prefix compression for B-tree keys.
 *
 * Computes minimum separators for internal nodes and compresses keys
 * by storing only the portion that differs from the predecessor. This
 * reduces B-tree fan-out overhead for workloads with correlated keys.
 */

#include "btree_compression.h"
#include "key_encoding.h"
#include <string.h>

garry_i32 garry_common_prefix_length(const garry_byte *a, garry_i32 alen, const garry_byte *b,
                                     garry_i32 blen)
{
    garry_i32 i, limit;
    limit = (alen < blen) ? alen : blen;
    i = 0;
    while (i < limit && a[i] == b[i])
    {
        i = i + 1;
    }
    return i;
}

void garry_minimum_separator(garry_byte_array result, const garry_byte *left, garry_i32 llen,
                             const garry_byte *right, garry_i32 rlen)
{
    garry_i32 plen, sep_len, i;
    plen = garry_common_prefix_length(left, llen, right, rlen);
    sep_len = plen + 1;
    if (sep_len > rlen)
    {
        sep_len = rlen;
    }
    memset(result, 0, sizeof(garry_byte_array));
    for (i = 0; i < sep_len; i++)
    {
        result[i] = right[i];
    }
    if (garry_byte_compare(result, sep_len, left, llen) <= 0)
    {
        /* Separator must be strictly greater than left.
         * Use the full left key as the separator. */
        sep_len = llen;
        if (sep_len > GARRY_MAX_KEY_SIZE)
            sep_len = GARRY_MAX_KEY_SIZE;
        for (i = 0; i < sep_len; i++)
        {
            result[i] = left[i];
        }
    }
}

void garry_compress_key(garry_byte_array result, const garry_byte *full_key, garry_i32 klen,
                        garry_i32 prefix_len)
{
    garry_i32 suffix_len, i;
    suffix_len = klen - prefix_len;
    memset(result, 0, sizeof(garry_byte_array));
    result[0] = (garry_byte)prefix_len;
    result[1] = (garry_byte)suffix_len;
    for (i = 0; i < suffix_len; i++)
    {
        result[2 + i] = full_key[prefix_len + i];
    }
}

void garry_decompress_key(garry_byte_array result, const garry_byte *compressed, garry_i32 clen,
                          const garry_byte *prev_key, garry_i32 prev_len)
{
    garry_i32 prefix_len, suffix_len, i;
    (void)clen;
    (void)prev_len;
    prefix_len = (garry_i32)compressed[0];
    suffix_len = (garry_i32)compressed[1];
    memset(result, 0, sizeof(garry_byte_array));
    for (i = 0; i < prefix_len; i++)
    {
        result[i] = prev_key[i];
    }
    for (i = 0; i < suffix_len; i++)
    {
        result[prefix_len + i] = compressed[2 + i];
    }
}

garry_bool garry_byte_equal(const garry_byte *a, garry_i32 alen, const garry_byte *b,
                            garry_i32 blen)
{
    garry_i32 i;
    if (alen != blen)
        return 0;
    for (i = 0; i < alen; i++)
    {
        if (a[i] != b[i])
            return 0;
    }
    return 1;
}
