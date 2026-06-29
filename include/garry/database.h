/**
 * @file database.h
 * @brief Database lifecycle management for the Garry storage engine.
 *
 * Provides functions to create, open, and close databases, plus the
 * @ref garry_config structure for tuning pool sizes, page sizes, and
 * transaction limits at open time.
 *
 * Intent: Expose an opaque handle (@ref garry_database) so callers never
 * depend on internal struct layouts.
 * Rationale: An opaque handle isolates the public API from implementation
 * changes, enabling ABI stability across minor versions.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_DATABASE_H
#define GARRY_DATABASE_H

#include "garry/export.h"
#include "garry/types.h"

/// Opaque database handle — the top-level container for all data.
#ifndef GARRY_DATABASE_FWD_DEFINED
#define GARRY_DATABASE_FWD_DEFINED
typedef struct garry_database garry_database;
#endif

/**
 * @brief Runtime configuration for a Garry database.
 *
 * Pass this to @ref garry_database_create_with_config to override defaults.
 * Use @ref garry_config_default to obtain a sensible starting point.
 */
typedef struct
{
    garry_i32 pool_size;       ///< Number of pages in the in-memory pool.
    garry_i32 max_record_size; ///< Maximum record size in bytes.
    garry_i32 page_size;       ///< Page size in bytes (must be power of two).
    garry_i32 max_txns;        ///< Maximum concurrent transactions.
    garry_i32 max_versions;    ///< Maximum version-chain depth per key.
    garry_i32 max_key_size;    ///< Maximum size of a user-supplied key in bytes.
    garry_i32 max_subscripts;  ///< Maximum number of subscripts in a composite key.
    garry_i32
        compression; ///< Compression mode (@ref GARRY_COMPRESS_NONE or @ref GARRY_COMPRESS_LZ4).
    garry_i32 btree_flags; ///< B-tree behavior flags.
} garry_config;

/**
 * @brief Return a @ref garry_config populated with engine defaults.
 * @return Default configuration suitable for most workloads.
 */
GARRY_API garry_config garry_config_default(void);

/**
 * @brief Create a new database at the given path, overwriting if it exists.
 * @param path  Filesystem path for the database file (e.g. "my.db").
 * @return Database handle, or @c NULL on failure.
 *
 * Creates a fresh database. If the file already exists it is truncated.
 * Use @ref garry_database_open to open an existing database without
 * truncating.
 */
GARRY_API garry_database *garry_database_create(const char *path);

/**
 * @brief Create a new database with custom configuration.
 * @param path    Filesystem path for the database file.
 * @param config  Runtime configuration (see @ref garry_config).
 * @return Database handle, or @c NULL on failure.
 *
 * Like @ref garry_database_create but allows tuning pool size, page size,
 * and transaction limits before the first page is written.
 */
GARRY_API garry_database *garry_database_create_with_config(const char *path, garry_config config);

/**
 * @brief Open an existing database.
 * @param path  Filesystem path of an existing database file.
 * @return Database handle, or @c NULL if the file does not exist or is corrupt.
 *
 * Opens the database in read/write mode. The file must have been created
 * previously with @ref garry_database_create or
 * @ref garry_database_create_with_config.
 */
GARRY_API garry_database *garry_database_open(const char *path);

/**
 * @brief Close a database and release all associated resources.
 * @param db  Database handle obtained from create/open.
 *
 * Flushes pending writes, releases the pool, and frees the handle.
 * After this call @p db is invalid and must not be used.
 */
GARRY_API void garry_database_close(garry_database *db);

#endif /* GARRY_DATABASE_H */
