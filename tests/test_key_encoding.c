/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "key_encoding.h"
#include "test_helpers.h"

int main(void)
{
    garry_byte_array k1, k2;
    garry_key_tuple decoded, t2;
    garry_i32 v0;
    garry_byte_array prefix_a, prefix_b;
    garry_i32 cmp;

    { garry_key_tuple t3 = garry_make_key_3("accounts", "user1", "balance"); garry_encode_key_tuple(&t3, k1); }

    v0 = garry_decode_length_prefix(k1, 0);
    GARRY_CHECK(v0 == 8);

    v0 = garry_decode_length_prefix(k1, 9);
    GARRY_CHECK(v0 == 5);

    v0 = garry_decode_length_prefix(k1, 15);
    GARRY_CHECK(v0 == 7);

    v0 = garry_length_prefix_size(8);
    GARRY_CHECK(v0 == 1);

    v0 = garry_length_prefix_size(200);
    GARRY_CHECK(v0 == 3);

    /* Regression: decode length >= 128 uses 255-marker form. */
    garry_empty_byte_array(prefix_a);
    prefix_a[0] = (garry_byte)255;
    prefix_a[1] = (garry_byte)0;
    prefix_a[2] = (garry_byte)200;
    v0 = garry_decode_length_prefix(prefix_a, 0);
    GARRY_CHECK(v0 == 200);

    t2 = garry_make_key_2("table", "row");
    v0 = garry_tuple_length(&t2);
    GARRY_CHECK(v0 == 10);

    { garry_key_tuple t2e = garry_make_key_2("table", "row"); garry_encode_key_tuple(&t2e, k2); }
    v0 = garry_decode_length_prefix(k2, 0);
    GARRY_CHECK(v0 == 5);

    v0 = garry_decode_length_prefix(k2, 6);
    GARRY_CHECK(v0 == 3);

    decoded = garry_decode_key_tuple(k1, 1 * 3 + 8 + 5 + 7);
    GARRY_CHECK(decoded.count == 3);
    GARRY_CHECK(decoded.counts[0] == 8);
    GARRY_CHECK(decoded.counts[1] == 5);
    GARRY_CHECK(decoded.counts[2] == 7);

    /* HAS_PREFIX negative tests. */
    garry_empty_byte_array(prefix_a);
    { garry_i32 vi; for (vi = 0; vi < 5; vi++) prefix_a[vi] = BYTE_FROM_INT(97 + vi); }
    GARRY_CHECK(garry_has_prefix(k1, 8, prefix_a, 5) == 0);

    garry_empty_byte_array(prefix_b);
    { garry_i32 vi; for (vi = 0; vi < 5; vi++) prefix_b[vi] = BYTE_FROM_INT(122); }
    GARRY_CHECK(garry_has_prefix(k1, 8, prefix_b, 5) == 0);

    /* BYTE_COMPARE. */
    cmp = garry_byte_compare(k1, 10, k2, 10);
    GARRY_CHECK(cmp != 0);

    cmp = garry_byte_compare(k1, 10, k1, 10);
    GARRY_CHECK(cmp == 0);

    return garry_test_failures;
}
