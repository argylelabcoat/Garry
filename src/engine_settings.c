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
#include "garry/config.h"

/**
 * @brief Return the default engine settings.
 */
garry_engine_settings garry_default_engine_settings(void)
{
    garry_engine_settings s;
    s.page_size       = GARRY_DEFAULT_PAGE_SIZE;
    s.compression     = GARRY_COMPRESS_NONE;
    s.max_txns        = GARRY_DEFAULT_MAX_TXNS;
    s.max_versions    = GARRY_DEFAULT_MAX_VERSIONS;
    s.max_key_size    = GARRY_DEFAULT_MAX_KEY_SIZE;
    s.max_subscripts  = GARRY_DEFAULT_MAX_SUBSCRIPTS;
    s.btree_flags     = 0;
    s.pool_size       = GARRY_DEFAULT_POOL_SIZE;
    s.max_record_size = GARRY_DEFAULT_MAX_RECORD_SIZE;
    return s;
}
