/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file engine_settings.c
 * @brief Default engine settings factory.
 *
 * Provides a single function that returns a fully-populated
 * garry_engine_settings with production-ready defaults for pool
 * size, page size, record limits, and transaction/version caps.
 */

#include "engine_settings.h"

/**
 * @brief Return the default engine settings.
 *
 * Default values: pool_size=64 pages, page_size=4096,
 * max_record_size=512, max_txns=4, max_versions=8.
 */
garry_engine_settings garry_default_engine_settings(void)
{
    garry_engine_settings s;
    s.page_size       = 4096;
    s.compression     = 0;
    s.max_txns        = 4;
    s.max_versions    = 64;
    s.max_key_size    = 256;
    s.max_subscripts  = 16;
    s.btree_flags     = 0;
    s.pool_size       = 8;
    s.max_record_size = 8192;
    return s;
}
