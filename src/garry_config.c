/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file garry_config.c
 * @brief Default configuration factory for the storage engine.
 *
 * Bridges engine_settings (internal) to garry_config (public API),
 * providing callers with a sensible starting configuration.
 */

#include "garry/database.h"
#include "engine_settings.h"

/**
 * @brief Return a garry_config populated with default engine settings.
 *
 * Copies pool size, record size, page size, max transactions, and
 * max versions from the engine defaults into the public config struct.
 */
garry_config garry_config_default(void)
{
    garry_config c;
    garry_engine_settings s = garry_default_engine_settings();
    c.pool_size       = s.pool_size;
    c.max_record_size = s.max_record_size;
    c.page_size       = s.page_size;
    c.max_txns        = s.max_txns;
    c.max_versions    = s.max_versions;
    c.compression     = s.compression;
    c.btree_flags     = s.btree_flags;
    return c;
}
