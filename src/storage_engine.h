/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file storage_engine.h
 * @brief Storage engine teardown and lifecycle helpers.
 */

#ifndef GARRY_STORAGE_ENGINE_H
#define GARRY_STORAGE_ENGINE_H

#include "transaction.h"

garry_engine_handle* garry_storage_init(const char *path, garry_engine_settings settings);
garry_engine_handle* garry_storage_open(const char *path);
void garry_storage_close(garry_engine_handle *eng);

#endif /* GARRY_STORAGE_ENGINE_H */
