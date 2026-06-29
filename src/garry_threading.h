/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file garry_threading.h
 * @brief Platform-agnostic mutex and reader-writer lock abstractions.
 *
 * Delegates to the platform-specific header for type definitions
 * and function implementations.
 */

#ifndef GARRY_THREADING_H
#define GARRY_THREADING_H

#include "garry/types.h"
#include "../platform_src/platform_threading.h"
#include "../platform_src/posix/threading_impl.h"

#endif
