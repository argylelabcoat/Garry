/**
 * @file types.h
 * @brief Foundational types for the Garry storage engine.
 *
 * Defines fixed-width integer types (portable C89, no <stdint.h>), status
 * codes, boolean/byte aliases, and opaque handle forward declarations used
 * across every other public header.
 *
 * Intent: Provide a single source of truth for the type vocabulary so that
 * all other Garry headers can depend on a consistent set of primitives.
 * Rationale: C89 lacks <stdint.h>; these typedefs make the API portable
 * across platforms without requiring a C99+ compiler.
 *
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */
#ifndef GARRY_TYPES_H
#define GARRY_TYPES_H

#include "garry/config.h"
#include <limits.h>

/* ---- Fixed-width integer types (C89 has no <stdint.h>) ---- */

/// Signed 8-bit integer.
typedef signed   char garry_i8;
/// Unsigned 8-bit integer.
typedef unsigned char garry_u8;

#if USHRT_MAX == 0xFFFFu
  /// Unsigned 16-bit integer.
  typedef unsigned short garry_u16;
#elif UINT_MAX == 0xFFFFu
  typedef unsigned int   garry_u16;
#else
#  error "Garry: no 16-bit unsigned integer type available"
#endif

#if UINT_MAX == 0xFFFFFFFFu
  /// Unsigned 32-bit integer.
  typedef unsigned int   garry_u32;
#elif ULONG_MAX == 0xFFFFFFFFu
  typedef unsigned long  garry_u32;
#else
#  error "Garry: no 32-bit unsigned integer type available"
#endif

#if INT_MAX == 0x7FFFFFFF
  /// Signed 32-bit integer.
  typedef int            garry_i32;
#elif LONG_MAX == 0x7FFFFFFFL
  typedef long           garry_i32;
#else
#  error "Garry: no 32-bit signed integer type available"
#endif

/// Unsigned byte — alias for @ref garry_u8, used for raw data.
typedef garry_u8 garry_byte;
/// Boolean type — 0 is false, nonzero is true.
typedef int      garry_bool;
/// Fixed-size byte buffer for keys and values (sized to @ref GARRY_MAX_KEY_SIZE).
typedef garry_u8 garry_byte_array[GARRY_MAX_KEY_SIZE];

/* ---- Compression modes ---- */

/// No compression (default).
#define GARRY_COMPRESS_NONE 0
/// LZ4 block compression.
#define GARRY_COMPRESS_LZ4  1

/* ---- Status codes ---- */

/**
 * @brief Error/status codes returned by all Garry operations.
 *
 * Every function that can fail returns a @ref garry_status_t. Check the
 * return value before using any output parameters.
 */
typedef enum {
    GARRY_OK = 0,                ///< Success.
    GARRY_ERR_NOT_FOUND,         ///< Key does not exist.
    GARRY_ERR_LOCK_CONFLICT,     ///< Transaction lock conflict.
    GARRY_ERR_IO,                ///< I/O error (disk failure, write error).
    GARRY_ERR_CORRUPT,           ///< Database corruption detected.
    GARRY_ERR_INVALID_ARG,       ///< Invalid argument passed to function.
    GARRY_ERR_NOMEM,             ///< Out of memory.
    GARRY_ERR_BUFFER_TOO_SMALL   ///< Output buffer is too small for result.
} garry_status_t;

/* ---- Opaque public handles (definitions live in src/) ---- */

/// Opaque handle to a storage engine instance.
typedef struct garry_storage garry_storage_t;
/// Opaque handle to an active transaction.
typedef struct garry_txn     garry_txn_t;
/// Opaque handle to a cursor for prefix-scoped iteration.
typedef struct garry_cursor  garry_cursor_t;

/**
 * @brief Return a human-readable string for a status code.
 * @param status  The status code to convert.
 * @return Pointer to a static string (e.g. "OK", "not found"). Never NULL.
 */
const char* garry_strerror(garry_status_t status);

#endif /* GARRY_TYPES_H */
