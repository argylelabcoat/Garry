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

/**
 * @brief Initialize a new storage engine at the given path.
 *
 * Creates the database file and initializes all engine subsystems
 * (buffer pool, B-tree, WAL, MVCC) with the provided settings.
 *
 * @param path      Filesystem path for the database file
 * @param settings  Engine configuration settings
 * @return Engine handle, or NULL on failure
 */
garry_engine_handle *garry_storage_init(const char *path, garry_engine_settings settings);

/**
 * @brief Open an existing storage engine.
 *
 * Opens the database file at @p path and restores engine state from
 * the persisted header.
 *
 * @param path  Filesystem path of the database file
 * @return Engine handle, or NULL on failure
 */
garry_engine_handle *garry_storage_open(const char *path);

/**
 * @brief Close the storage engine and flush all pending state.
 *
 * Writes the updated database header, flushes all dirty buffer pool
 * pages, syncs the WAL, and releases all engine resources.
 *
 * @param eng  Storage engine handle to close
 */
void garry_storage_close(garry_engine_handle *eng);

#endif /* GARRY_STORAGE_ENGINE_H */
