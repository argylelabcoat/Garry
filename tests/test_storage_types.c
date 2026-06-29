/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#include "storage_types.h"
#include "test_helpers.h"

int main(void)
{
    GARRY_CHECK(GARRY_MAX_TXNS == 4);
    GARRY_CHECK(sizeof(garry_byte_array) == 256);
    GARRY_CHECK(sizeof(garry_txn_set) == 16);
    return garry_test_failures;
}
