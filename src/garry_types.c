/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file garry_types.c
 * @brief Error code to string conversion for the Garry storage engine.
 *
 * Provides human-readable error messages for all status codes returned
 * by the storage engine API, enabling consistent error reporting.
 */

#include "garry/types.h"

/**
 * @brief Convert a status code to its string representation.
 *
 * @param status  The error code to look up.
 * @return Pointer to a static string describing the error.
 */
const char* garry_strerror(garry_status_t status)
{
    switch (status) {
        case GARRY_OK:                   return "ok";
        case GARRY_ERR_NOT_FOUND:        return "not found";
        case GARRY_ERR_LOCK_CONFLICT:    return "lock conflict";
        case GARRY_ERR_IO:               return "i/o error";
        case GARRY_ERR_CORRUPT:          return "corrupt database";
        case GARRY_ERR_INVALID_ARG:      return "invalid argument";
        case GARRY_ERR_NOMEM:            return "out of memory";
        case GARRY_ERR_BUFFER_TOO_SMALL: return "buffer too small";
        default:                         return "unknown error";
    }
}
