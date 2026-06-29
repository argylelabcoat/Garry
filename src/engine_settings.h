/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file engine_settings.h
 * @brief Internal engine settings (pool size, page size, limits).
 */

#ifndef GARRY_ENGINE_SETTINGS_H
#define GARRY_ENGINE_SETTINGS_H

#include "garry/types.h"

#define GARRY_MAX_TXNS_CAPACITY       64
#define GARRY_MAX_VERSIONS_CAPACITY   256
#define GARRY_MAX_KEY_SIZE_CAPACITY   256
#define GARRY_MAX_SUBSCRIPTS_CAPACITY 32

#define GARRY_COMPRESS_NONE 0
#define GARRY_COMPRESS_LZ4  1

#define GARRY_BTREE_FLAG_COMPRESS_KEYS 1

typedef struct
{
    garry_i32 page_size;
    garry_i32 compression;
    garry_i32 max_txns;
    garry_i32 max_versions;
    garry_i32 max_key_size;
    garry_i32 max_subscripts;
    garry_i32 btree_flags;
    garry_i32 pool_size;
    garry_i32 max_record_size;
} garry_engine_settings;

/**
 * @brief Create default engine settings with standard configuration.
 *
 * Returns a settings struct initialized with default values for page size,
 * compression mode, transaction limits, and B-tree parameters.
 *
 * @return Settings struct populated with default values (passed by value)
 */
garry_engine_settings garry_default_engine_settings(void);

#endif /* GARRY_ENGINE_SETTINGS_H */
