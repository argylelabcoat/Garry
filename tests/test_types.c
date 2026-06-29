/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "garry/types.h"
#include "test_helpers.h"
#include <limits.h>

int main(void)
{
    GARRY_CHECK(sizeof(garry_u8) == 1);
    GARRY_CHECK(sizeof(garry_i8) == 1);
    GARRY_CHECK(sizeof(garry_u16) == 2);
    GARRY_CHECK(sizeof(garry_u32) == 4);
    GARRY_CHECK(sizeof(garry_i32) == 4);
    GARRY_CHECK(sizeof(garry_byte) == 1);
    GARRY_CHECK(((garry_u16)~0) == 0xFFFFu);
    GARRY_CHECK(((garry_u32)~0) == 0xFFFFFFFFul);
    GARRY_CHECK(GARRY_OK == 0);
    GARRY_CHECK(garry_strerror(GARRY_OK) != 0);
    GARRY_CHECK(garry_strerror(GARRY_ERR_IO) != 0);
    GARRY_CHECK(garry_strerror(GARRY_ERR_NOT_FOUND) != 0);
    return garry_test_failures;
}
