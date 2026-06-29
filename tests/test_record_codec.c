/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "record_codec.h"
#include "key_encoding.h"
#include "test_helpers.h"

int main(void)
{
    garry_byte_array key_in, val_in, key_out, val_out, key_dec;
    garry_byte buf[GARRY_MAX_RECORD_SIZE];
    garry_i32 len, klen_out, vlen_out, i;
    garry_i32 key_only_len, kdec_len;
    garry_i32 cid_out;
    garry_bool hc_out;

    memset(key_in, 0, sizeof(key_in));
    memset(val_in, 0, sizeof(val_in));
    for (i = 0; i < 8; i++) {
        key_in[i] = (garry_byte)(97 + i);
        val_in[i] = (garry_byte)(65 + i);
    }
    len = garry_encode_kv(key_in, 8, val_in, 8, buf);
    GARRY_CHECK(len > 0);
    memset(key_out, 0, sizeof(key_out));
    memset(val_out, 0, sizeof(val_out));
    garry_decode_kv(buf, len, key_out, &klen_out, val_out, &vlen_out);
    GARRY_CHECK(klen_out == 8);
    GARRY_CHECK(vlen_out == 8);
    for (i = 0; i < 8; i++) {
        GARRY_CHECK(key_out[i] == key_in[i]);
        GARRY_CHECK(val_out[i] == val_in[i]);
    }

    key_only_len = garry_encode_key_only(key_in, 8, buf);
    GARRY_CHECK(key_only_len > 0);
    memset(key_dec, 0, sizeof(key_dec));
    kdec_len = garry_decode_key_only(buf, key_only_len, key_dec);
    GARRY_CHECK(kdec_len == 8);
    for (i = 0; i < 8; i++) {
        GARRY_CHECK(key_dec[i] == key_in[i]);
    }

    len = garry_encode_descriptor(5, 1, buf);
    GARRY_CHECK(len > 0);
    garry_decode_descriptor(buf, len, &cid_out, &hc_out);
    GARRY_CHECK(cid_out == 5);
    GARRY_CHECK(hc_out == 1);

    len = garry_encode_descriptor(-1, 0, buf);
    GARRY_CHECK(len > 0);
    garry_decode_descriptor(buf, len, &cid_out, &hc_out);
    GARRY_CHECK(cid_out == -1);
    GARRY_CHECK(hc_out == 0);

    len = garry_encode_descriptor(0, 1, buf);
    garry_decode_descriptor(buf, len, &cid_out, &hc_out);
    GARRY_CHECK(cid_out == 0);
    GARRY_CHECK(hc_out == 1);

    return garry_test_failures;
}
