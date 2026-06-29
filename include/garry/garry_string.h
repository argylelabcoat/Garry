/**
 * @file garry_string.h
 * @brief String convenience functions for the Garry storage engine.
 *
 * Provides @ref garry_set_str and @ref garry_get_str — thin wrappers
 * around @ref garry_set and @ref garry_get that accept and return
 * null-terminated C strings instead of raw byte buffers.
 *
 * Intent: Reduce boilerplate for the common case of storing and
 * retrieving text values.
 * Rationale: Most callers work with C strings; forcing them to compute
 * lengths and pass byte buffers for simple string storage is tedious
 * and error-prone.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_STRING_H
#define GARRY_STRING_H

#include "garry/export.h"
#include "garry/types.h"

/// Forward declaration — defined in database.h.
#ifndef GARRY_DATABASE_FWD_DEFINED
#define GARRY_DATABASE_FWD_DEFINED
typedef struct garry_database garry_database;
#endif

/**
 * @brief Store a string value under a string key.
 * @param db     Database handle.
 * @param txn    Active transaction handle.
 * @param key    Null-terminated key string.
 * @param value  Null-terminated value string.
 * @return @ref GARRY_OK on success, or another status code on error.
 *
 * Convenience wrapper: computes key and value lengths automatically and
 * delegates to @ref garry_set. The null terminators are not stored.
 */
GARRY_API garry_status_t garry_set_str(garry_database *db, garry_txn txn,
                                         const char *key, const char *value);

/**
 * @brief Retrieve a string value for a string key.
 * @param db          Database handle.
 * @param txn         Active transaction handle.
 * @param key         Null-terminated key string.
 * @param value       Output buffer for the null-terminated value.
 * @param value_size  Capacity of @p value in bytes.
 * @return @ref GARRY_OK on success, @ref GARRY_ERR_NOT_FOUND if the key
 *         is absent, @ref GARRY_ERR_BUFFER_TOO_SMALL if @p value_size is
 *         insufficient, or another status code on error.
 *
 * The retrieved value is null-terminated in @p value. If the stored value
 * fills the entire buffer, the last byte is set to '\0' to guarantee
 * null termination.
 */
GARRY_API garry_status_t garry_get_str(garry_database *db, garry_txn txn,
                                         const char *key, char *value,
                                         garry_i32 value_size);

#endif /* GARRY_STRING_H */
