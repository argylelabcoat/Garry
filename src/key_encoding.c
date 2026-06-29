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

garry_i32 garry_tuple_length(garry_key_tuple* t)
{
    return t->count;
}

garry_key_tuple garry_make_key_2(const char* p1, const char* p2)
{
    garry_key_tuple t;
    t.count = 2;
    t.parts[0] = (const garry_byte*)p1;
    t.parts[1] = (const garry_byte*)p2;
    t.counts[0] = (garry_i32)strlen(p1);
    t.counts[1] = (garry_i32)strlen(p2);
    return t;
}

garry_key_tuple garry_make_key_3(const char* p1, const char* p2, const char* p3)
{
    garry_key_tuple t;
    t.count = 3;
    t.parts[0] = (const garry_byte*)p1;
    t.parts[1] = (const garry_byte*)p2;
    t.parts[2] = (const garry_byte*)p3;
    t.counts[0] = (garry_i32)strlen(p1);
    t.counts[1] = (garry_i32)strlen(p2);
    t.counts[2] = (garry_i32)strlen(p3);
    return t;
}

garry_key_tuple garry_make_key_4(const char* p1, const char* p2, const char* p3, const char* p4)
{
    garry_key_tuple t;
    t.count = 4;
    t.parts[0] = (const garry_byte*)p1;
    t.parts[1] = (const garry_byte*)p2;
    t.parts[2] = (const garry_byte*)p3;
    t.parts[3] = (const garry_byte*)p4;
    t.counts[0] = (garry_i32)strlen(p1);
    t.counts[1] = (garry_i32)strlen(p2);
    t.counts[2] = (garry_i32)strlen(p3);
    t.counts[3] = (garry_i32)strlen(p4);
    return t;
}

garry_i32 garry_encode_length_prefix(garry_byte_array result, garry_i32 offset, garry_i32 plen)
{
    if (plen < GARRY_LEN_PREFIX_INLINE_MAX) {
        result[offset] = (garry_byte)plen;
        return offset + 1;
    } else {
        result[offset] = (garry_byte)GARRY_LEN_PREFIX_LONG_MARKER;
        result[offset + 1] = (garry_byte)((plen / 256) % 256);
        result[offset + 2] = (garry_byte)(plen % 256);
        return offset + 3;
    }
}

garry_i32 garry_decode_length_prefix(const garry_byte* key, garry_i32 offset)
{
    garry_i32 v0, v1, v2;
    v0 = (garry_i32)key[offset];
    if (v0 != GARRY_LEN_PREFIX_LONG_MARKER) {
        return v0;
    }
    v1 = (garry_i32)key[offset + 1];
    v2 = (garry_i32)key[offset + 2];
    return v1 * 256 + v2;
}

garry_i32 garry_length_prefix_size(garry_i32 plen)
{
    return (plen < GARRY_LEN_PREFIX_INLINE_MAX) ? 1 : 3;
}

void garry_encode_key_tuple(garry_key_tuple* t, garry_byte_array out)
{
    garry_i32 offset, i, plen, j;
    memset(out, 0, sizeof(garry_byte_array));
    offset = 0;
    for (i = 0; i < t->count; i++) {
        plen = t->counts[i];
        offset = garry_encode_length_prefix(out, offset, plen);
        for (j = 0; j < plen; j++) {
            out[offset] = t->parts[i][j];
            offset = offset + 1;
        }
    }
}

garry_key_tuple garry_decode_key_tuple(const garry_byte* encoded, garry_i32 elen)
{
    garry_key_tuple result;
    garry_i32 offset, idx, plen, hdr_size;
    result.count = 0;
    offset = 0;
    idx = 0;
    while (offset < elen) {
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

garry_bool garry_has_prefix(const garry_byte* key, garry_i32 klen,
                            const garry_byte* prefix, garry_i32 plen)
{
    garry_i32 i;
    if (klen < plen) {
        return 0;
    }
    for (i = 0; i < plen; i++) {
        if (key[i] != prefix[i]) {
            return 0;
        }
    }
    return 1;
}

void garry_empty_byte_array(garry_byte_array out)
{
    memset(out, 0, sizeof(garry_byte_array));
}

void garry_encode_integer_subscript(garry_i32 n, garry_byte_array out)
{
    garry_i32 i;
    garry_i32 val;
    memset(out, 0, sizeof(garry_byte_array));
    out[0] = (garry_byte)2;
    val = n;
    /* Write 8 little-endian bytes starting at index 1. */
    for (i = 1; i <= 8; i++) {
        out[i] = (garry_byte)(val % 256);
        val = val / 256;
    }
}

garry_i32 garry_decode_integer_subscript(const garry_byte* encoded, garry_i32 offset)
{
    garry_i32 result;
    garry_i32 i;
    garry_i32 byte_val;
    result = 0;
    /* Read 8 little-endian bytes. */
    for (i = 8; i >= 1; i--) {
        byte_val = (garry_i32)encoded[offset + i];
        result = result * 256 + byte_val;
    }
    return result;
}

garry_i32 garry_byte_compare(const garry_byte* a, garry_i32 alen,
                             const garry_byte* b, garry_i32 blen)
{
    garry_i32 i, limit;
    limit = (alen < blen) ? alen : blen;
    for (i = 0; i < limit; i++) {
        if ((garry_u8)a[i] < (garry_u8)b[i]) return -1;
        if ((garry_u8)a[i] > (garry_u8)b[i]) return 1;
    }
    if (alen < blen) return -1;
    if (alen > blen) return 1;
    return 0;
}
