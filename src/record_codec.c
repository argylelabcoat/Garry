/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file record_codec.c
 * @brief CBOR-based encoding for key-value records and version descriptors.
 *
 * Key-value records are encoded as CBOR arrays of two byte strings:
 *   [0x82, key_header, key_bytes, val_header, val_bytes]
 *
 * Version descriptors encode a 2-entry CBOR map with keys "cid" and "hc":
 *   cid = chain ID (uint or nint), hc = has-children (simple true/false).
 *
 * All encoding is position-tracking (no intermediate allocations).
 */

#include "record_codec.h"
#include <string.h>

garry_i32 garry_cbor_encode_array_header_raw(garry_byte* out, garry_i32 pos, garry_i32 count)
{
    if (count < 24) {
        out[pos] = (garry_byte)(GARRY_CBOR_ARRAY_BASE + count);
        return pos + 1;
    } else {
        out[pos] = (garry_byte)(GARRY_CBOR_ARRAY_BASE + 24);
        out[pos + 1] = (garry_byte)count;
        return pos + 2;
    }
}

garry_i32 garry_cbor_encode_byte_string_raw(const garry_byte* data, garry_i32 dlen,
                                            garry_byte* out, garry_i32 pos)
{
    if (dlen < 24) {
        out[pos] = (garry_byte)(GARRY_CBOR_BYTES_BASE + dlen);
        pos = pos + 1;
    } else {
        out[pos] = (garry_byte)(GARRY_CBOR_BYTES_BASE + 24);
        out[pos + 1] = (garry_byte)dlen;
        pos = pos + 2;
    }
    memcpy(out + pos, data, (size_t)dlen);
    return pos + dlen;
}

garry_i32 garry_cbor_decode_array_header_raw(const garry_byte* buf, garry_i32 blen, garry_i32 pos)
{
    garry_i32 fb, ai;
    fb = (garry_i32)buf[pos];
    if (fb < 0) fb += 256;
    if (fb < GARRY_CBOR_ARRAY_BASE || fb >= GARRY_CBOR_ARRAY_BASE + 32) {
        return 0;
    }
    ai = fb - GARRY_CBOR_ARRAY_BASE;
    if (ai < 24) {
        return ai;
    } else if (ai == 24 && pos + 1 < blen) {
        return (garry_i32)buf[pos + 1];
    }
    return 0;
}

garry_i32 garry_cbor_array_header_size_raw(const garry_byte* buf, garry_i32 pos)
{
    garry_i32 fb, ai;
    fb = (garry_i32)buf[pos];
    if (fb < 0) fb += 256;
    ai = fb - GARRY_CBOR_ARRAY_BASE;
    if (ai < 24) return 1;
    if (ai == 24) return 2;
    return 1;
}

garry_i32 garry_cbor_decode_byte_string_raw(const garry_byte* buf, garry_i32 blen, garry_i32 pos,
                                            garry_byte* out_data, garry_i32* dlen)
{
    garry_i32 fb, ai, slen, i;
    fb = (garry_i32)buf[pos];
    if (fb < 0) fb += 256;
    ai = fb - GARRY_CBOR_BYTES_BASE;
    if (ai < 24) {
        slen = ai;
        pos = pos + 1;
    } else if (ai == 24 && pos + 1 < blen) {
        slen = (garry_i32)buf[pos + 1];
        pos = pos + 2;
    } else {
        *dlen = 0;
        return 0;
    }
    for (i = 0; i < slen; i++) {
        out_data[i] = buf[pos + i];
    }
    *dlen = slen;
    return pos + slen;
}

garry_i32 garry_encode_kv(const garry_byte* key, garry_i32 klen,
                          const garry_byte* value, garry_i32 vlen,
                          garry_byte* out_buf)
{
    garry_i32 pos;
    pos = 0;
    pos = garry_cbor_encode_array_header_raw(out_buf, pos, 2);
    pos = garry_cbor_encode_byte_string_raw(key, klen, out_buf, pos);
    if (pos <= 0) return 0;
    pos = garry_cbor_encode_byte_string_raw(value, vlen, out_buf, pos);
    if (pos <= 0) return 0;
    return pos;
}

void garry_decode_kv(const garry_byte* encoded, garry_i32 elen,
                     garry_byte* key, garry_i32* klen,
                     garry_byte* value, garry_i32* vlen)
{
    garry_i32 pos, arr_count;
    pos = 0;
    *klen = 0;
    *vlen = 0;
    arr_count = garry_cbor_decode_array_header_raw(encoded, elen, pos);
    if (arr_count != 2) {
        *klen = 0;
        *vlen = 0;
        return;
    }
    pos = pos + garry_cbor_array_header_size_raw(encoded, pos);
    pos = garry_cbor_decode_byte_string_raw(encoded, elen, pos, key, klen);
    if (pos <= 0) {
        *klen = 0;
        *vlen = 0;
        return;
    }
    pos = garry_cbor_decode_byte_string_raw(encoded, elen, pos, value, vlen);
    if (pos <= 0) {
        *klen = 0;
        *vlen = 0;
        return;
    }
}

garry_i32 garry_encode_key_only(const garry_byte* key, garry_i32 klen,
                                garry_byte* out_buf)
{
    return garry_cbor_encode_byte_string_raw(key, klen, out_buf, 0);
}

garry_i32 garry_decode_key_only(const garry_byte* encoded, garry_i32 elen,
                                garry_byte* key)
{
    garry_i32 klen;
    garry_cbor_decode_byte_string_raw(encoded, elen, 0, key, &klen);
    return klen;
}

static garry_i32 encode_uint(garry_byte* out, garry_i32 pos, garry_i32 val)
{
    if (val >= 0 && val < 24) {
        out[pos] = (garry_byte)(GARRY_CBOR_UINT_BASE + val);
        return pos + 1;
    } else if (val >= 0 && val < 256) {
        out[pos] = (garry_byte)(GARRY_CBOR_UINT_BASE + 24);
        out[pos + 1] = (garry_byte)val;
        return pos + 2;
    } else if (val >= 0 && val < 65536) {
        out[pos] = (garry_byte)(GARRY_CBOR_UINT_BASE + 25);
        out[pos + 1] = (garry_byte)((val / 256) % 256);
        out[pos + 2] = (garry_byte)(val % 256);
        return pos + 3;
    } else {
        out[pos] = (garry_byte)(GARRY_CBOR_UINT_BASE + 26);
        out[pos + 1] = (garry_byte)((val / 16777216) % 256);
        out[pos + 2] = (garry_byte)((val / 65536) % 256);
        out[pos + 3] = (garry_byte)((val / 256) % 256);
        out[pos + 4] = (garry_byte)(val % 256);
        return pos + 5;
    }
}

static garry_i32 encode_negint(garry_byte* out, garry_i32 pos, garry_i32 abs_val)
{
    if (abs_val < 24) {
        out[pos] = (garry_byte)(GARRY_CBOR_NEGINT_BASE + abs_val);
        return pos + 1;
    } else if (abs_val < 256) {
        out[pos] = (garry_byte)(GARRY_CBOR_NEGINT_BASE + 24);
        out[pos + 1] = (garry_byte)abs_val;
        return pos + 2;
    } else if (abs_val < 65536) {
        out[pos] = (garry_byte)(GARRY_CBOR_NEGINT_BASE + 25);
        out[pos + 1] = (garry_byte)((abs_val / 256) % 256);
        out[pos + 2] = (garry_byte)(abs_val % 256);
        return pos + 3;
    } else {
        out[pos] = (garry_byte)(GARRY_CBOR_NEGINT_BASE + 26);
        out[pos + 1] = (garry_byte)((abs_val / 16777216) % 256);
        out[pos + 2] = (garry_byte)((abs_val / 65536) % 256);
        out[pos + 3] = (garry_byte)((abs_val / 256) % 256);
        out[pos + 4] = (garry_byte)(abs_val % 256);
        return pos + 5;
    }
}

/**
 * Encode a version descriptor as a CBOR map.
 *
 * Format: map(2) {
 *   "cid": uint/nint(chain_id),
 *   "hc":  simple(has_children)
 * }
 *
 * Positive chain_id encodes as CBOR uint, negative as CBOR nint.
 * Returns the number of bytes written to out_buf.
 */
garry_i32 garry_encode_descriptor(garry_i32 chain_id, garry_bool has_children,
                                  garry_byte* out_buf)
{
    garry_i32 pos, abs_val;
    pos = 0;
    out_buf[pos] = (garry_byte)(GARRY_CBOR_MAP_BASE + 2);
    pos = pos + 1;
    out_buf[pos] = (garry_byte)(GARRY_CBOR_TEXT_BASE + 3);
    pos = pos + 1;
    out_buf[pos] = (garry_byte)99;
    pos = pos + 1;
    out_buf[pos] = (garry_byte)105;
    pos = pos + 1;
    out_buf[pos] = (garry_byte)100;
    pos = pos + 1;
    if (chain_id >= 0) {
        pos = encode_uint(out_buf, pos, chain_id);
    } else {
        abs_val = -1 - chain_id;
        pos = encode_negint(out_buf, pos, abs_val);
    }
    out_buf[pos] = (garry_byte)(GARRY_CBOR_TEXT_BASE + 2);
    pos = pos + 1;
    out_buf[pos] = (garry_byte)104;
    pos = pos + 1;
    out_buf[pos] = (garry_byte)99;
    pos = pos + 1;
    if (has_children) {
        out_buf[pos] = (garry_byte)(GARRY_CBOR_SIMPLE_BASE + GARRY_CBOR_SIMPLE_TRUE);
    } else {
        out_buf[pos] = (garry_byte)(GARRY_CBOR_SIMPLE_BASE + GARRY_CBOR_SIMPLE_FALSE);
    }
    pos = pos + 1;
    return pos;
}

static garry_i32 decode_uint(const garry_byte* encoded, garry_i32 elen, garry_i32 pos, garry_i32* val)
{
    garry_i32 fb, b0, b1, b2, b3;
    fb = (garry_i32)encoded[pos];
    if (fb < 0) fb += 256;
    if (fb >= GARRY_CBOR_UINT_BASE && fb < GARRY_CBOR_UINT_BASE + 24) {
        *val = fb - GARRY_CBOR_UINT_BASE;
        return pos + 1;
    } else if (fb == GARRY_CBOR_UINT_BASE + 24 && pos + 1 < elen) {
        b0 = (garry_i32)encoded[pos + 1];
        if (b0 < 0) b0 += 256;
        *val = b0;
        return pos + 2;
    } else if (fb == GARRY_CBOR_UINT_BASE + 25 && pos + 2 < elen) {
        b0 = (garry_i32)encoded[pos + 1];
        b1 = (garry_i32)encoded[pos + 2];
        if (b0 < 0) b0 += 256;
        if (b1 < 0) b1 += 256;
        *val = b0 * 256 + b1;
        return pos + 3;
    } else if (fb == GARRY_CBOR_UINT_BASE + 26 && pos + 4 < elen) {
        b0 = (garry_i32)encoded[pos + 1];
        b1 = (garry_i32)encoded[pos + 2];
        b2 = (garry_i32)encoded[pos + 3];
        b3 = (garry_i32)encoded[pos + 4];
        if (b0 < 0) b0 += 256;
        if (b1 < 0) b1 += 256;
        if (b2 < 0) b2 += 256;
        if (b3 < 0) b3 += 256;
        *val = b0 * 16777216 + b1 * 65536 + b2 * 256 + b3;
        return pos + 5;
    }
    return -1;
}

static garry_i32 decode_negint(const garry_byte* encoded, garry_i32 elen, garry_i32 pos, garry_i32* val)
{
    garry_i32 fb, b0, b1, b2, b3, abs_val;
    fb = (garry_i32)encoded[pos];
    if (fb < 0) fb += 256;
    if (fb >= GARRY_CBOR_NEGINT_BASE && fb < GARRY_CBOR_NEGINT_BASE + 24) {
        *val = -1 - (fb - GARRY_CBOR_NEGINT_BASE);
        return pos + 1;
    } else if (fb == GARRY_CBOR_NEGINT_BASE + 24 && pos + 1 < elen) {
        b0 = (garry_i32)encoded[pos + 1];
        if (b0 < 0) b0 += 256;
        *val = -1 - b0;
        return pos + 2;
    } else if (fb == GARRY_CBOR_NEGINT_BASE + 25 && pos + 2 < elen) {
        b0 = (garry_i32)encoded[pos + 1];
        b1 = (garry_i32)encoded[pos + 2];
        if (b0 < 0) b0 += 256;
        if (b1 < 0) b1 += 256;
        *val = -1 - (b0 * 256 + b1);
        return pos + 3;
    } else if (fb == GARRY_CBOR_NEGINT_BASE + 26 && pos + 4 < elen) {
        b0 = (garry_i32)encoded[pos + 1];
        b1 = (garry_i32)encoded[pos + 2];
        b2 = (garry_i32)encoded[pos + 3];
        b3 = (garry_i32)encoded[pos + 4];
        if (b0 < 0) b0 += 256;
        if (b1 < 0) b1 += 256;
        if (b2 < 0) b2 += 256;
        if (b3 < 0) b3 += 256;
        abs_val = b0 * 16777216 + b1 * 65536 + b2 * 256 + b3;
        *val = -1 - abs_val;
        return pos + 5;
    }
    return -1;
}

/**
 * Decode a version descriptor from CBOR-encoded bytes.
 *
 * Parses the map(2) format written by garry_encode_descriptor.
 * Sets *chain_id to the decoded integer and *has_children from the
 * simple boolean value. Silently returns zeros on malformed input.
 */
void garry_decode_descriptor(const garry_byte* encoded, garry_i32 elen,
                             garry_i32* chain_id, garry_bool* has_children)
{
    garry_i32 pos, fb, text_len, next_pos;
    pos = 0;
    *chain_id = 0;
    *has_children = 0;
    if (pos >= elen) return;
    fb = (garry_i32)encoded[pos];
    if (fb < 0) fb += 256;
    if (fb < GARRY_CBOR_MAP_BASE || fb >= GARRY_CBOR_MAP_BASE + 32) {
        return;
    }
    pos = pos + 1;
    if (pos >= elen) return;
    fb = (garry_i32)encoded[pos];
    if (fb < 0) fb += 256;
    text_len = fb - GARRY_CBOR_TEXT_BASE;
    pos = pos + 1 + text_len;
    if (pos >= elen) return;
    fb = (garry_i32)encoded[pos];
    if (fb < 0) fb += 256;
    if (fb >= GARRY_CBOR_UINT_BASE && fb < GARRY_CBOR_UINT_BASE + 32) {
        next_pos = decode_uint(encoded, elen, pos, chain_id);
        if (next_pos > pos) pos = next_pos;
    } else if (fb >= GARRY_CBOR_NEGINT_BASE && fb < GARRY_CBOR_NEGINT_BASE + 32) {
        next_pos = decode_negint(encoded, elen, pos, chain_id);
        if (next_pos > pos) pos = next_pos;
    } else {
        return;
    }
    if (pos >= elen) return;
    fb = (garry_i32)encoded[pos];
    if (fb < 0) fb += 256;
    text_len = fb - GARRY_CBOR_TEXT_BASE;
    pos = pos + 1 + text_len;
    if (pos >= elen) return;
    fb = (garry_i32)encoded[pos];
    if (fb < 0) fb += 256;
    if (fb == GARRY_CBOR_SIMPLE_BASE + GARRY_CBOR_SIMPLE_TRUE) {
        *has_children = 1;
    } else {
        *has_children = 0;
    }
}
