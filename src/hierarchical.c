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

garry_i32 garry_subscript_count(const garry_byte *key, garry_i32 klen)
{
    garry_i32 count;
    garry_i32 offset;
    garry_i32 plen;
    garry_i32 hdr_size;
    count = 0;
    offset = 0;
    while (offset < klen) {
        plen = garry_decode_length_prefix(key, offset);
        hdr_size = garry_length_prefix_size(plen);
        offset = offset + hdr_size + plen;
        count = count + 1;
    }
    return count;
}

garry_i32 garry_get_subscript_offset(const garry_byte *key, garry_i32 klen, garry_i32 idx)
{
    garry_i32 offset;
    garry_i32 i;
    garry_i32 plen;
    garry_i32 hdr_size;
    offset = 0;
    for (i = 0; i < idx; i++) {
        if (offset >= klen) return -1;
        plen = garry_decode_length_prefix(key, offset);
        hdr_size = garry_length_prefix_size(plen);
        offset = offset + hdr_size + plen;
        if (offset > klen) return -1;
    }
    return offset;
}

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
    for (i = 0; i < idx; i++) {
        if (offset >= klen) return 0;
        plen = garry_decode_length_prefix(key, offset);
        hdr_size = garry_length_prefix_size(plen);
        offset = offset + hdr_size + plen;
        if (offset > klen) return 0;
    }
    if (offset >= klen) return 0;
    plen = garry_decode_length_prefix(key, offset);
    hdr_size = garry_length_prefix_size(plen);
    if (offset + hdr_size + plen > klen) return 0;
    offset = offset + hdr_size;
    exp_len = (garry_i32)strlen(expected);
    if (plen != exp_len) {
        return 0;
    }
    for (j = 0; j < plen; j++) {
        if ((garry_i32)key[offset + j] != (garry_i32)(unsigned char)expected[j]) {
            return 0;
        }
    }
    return 1;
}

garry_i32 garry_key_prefix(const garry_byte *key, garry_i32 klen, garry_i32 nsubs,
                           garry_byte *out, garry_i32 out_size)
{
    garry_i32 plen;
    garry_i32 i;
    plen = garry_get_subscript_offset(key, klen, nsubs);
    if (plen > out_size) return -1;
    memset(out, 0, (size_t)out_size);
    for (i = 0; i < plen; i++) {
        out[i] = key[i];
    }
    return plen;
}

void garry_extract_subscript(const garry_byte *key, garry_i32 klen, garry_i32 idx,
                             garry_byte *sub, garry_i32 *sub_len)
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
    for (j = 0; j < plen; j++) {
        sub[j] = key[src_off + j];
    }
}

garry_i32 garry_concat_prefix(const garry_byte *parent, garry_i32 parent_len,
                               const garry_byte *sub, garry_i32 sub_len,
                               garry_byte *out, garry_i32 out_size)
{
    garry_i32 offset;
    garry_i32 total;
    garry_i32 i;
    total = parent_len + garry_length_prefix_size(sub_len) + sub_len;
    if (total > out_size) return -1;
    memset(out, 0, (size_t)out_size);
    for (i = 0; i < parent_len; i++) {
        out[i] = parent[i];
    }
    offset = garry_encode_length_prefix(out, parent_len, sub_len);
    for (i = 0; i < sub_len; i++) {
        out[offset + i] = sub[i];
    }
    return total;
}
