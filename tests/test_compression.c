/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "btree_compression.h"
#include "key_encoding.h"
#include "test_helpers.h"

int main(void)
{
    garry_byte_array left, right, sep, key, comp, restored;
    garry_i32 plen, i;

    memset(left, 0, sizeof(left));
    memset(right, 0, sizeof(right));
    for (i = 0; i < 8; i++)
    {
        left[i] = BYTE_FROM_INT(97 + i);
        right[i] = BYTE_FROM_INT(97 + i);
    }
    plen = garry_common_prefix_length(left, 8, right, 8);
    GARRY_CHECK(plen == 8);

    right[7] = BYTE_FROM_INT(122);
    plen = garry_common_prefix_length(left, 8, right, 8);
    GARRY_CHECK(plen == 7);

    garry_minimum_separator(sep, left, 8, right, 8);
    GARRY_CHECK(sep[7] == BYTE_FROM_INT(122));

    memset(key, 0, sizeof(key));
    for (i = 0; i < 5; i++)
    {
        key[i] = BYTE_FROM_INT(117 + i);
    }
    garry_compress_key(comp, key, 5, 0);
    GARRY_CHECK(comp[0] == 0);
    GARRY_CHECK(comp[1] == 5);
    garry_decompress_key(restored, comp, 7, left, 0);
    GARRY_CHECK(restored[0] == 117);
    GARRY_CHECK(restored[4] == 121);

    GARRY_CHECK(garry_byte_equal(left, 8, left, 8) == 1);

    left[7] = BYTE_FROM_INT(97);
    right[7] = BYTE_FROM_INT(98);
    GARRY_CHECK(garry_byte_compare(left, 8, right, 8) < 0);

    GARRY_CHECK(garry_byte_equal(left, 8, left, 8) == 1);

    return garry_test_failures;
}
