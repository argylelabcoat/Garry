/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file win32/threading_impl.h
 * @brief Win32-specific type definitions for mutex and rwlock.
 *
 * Uses CRITICAL_SECTION for mutex (always available).
 * Uses SRWLOCK for reader-writer locks (Vista+). MinGW targets
 * Windows 7+ by default, so SRWLOCK is available.
 *
 * SRWLOCK has separate Release calls for shared vs exclusive.
 * Callers use garry_rwlock_rdunlock() / garry_rwlock_wrunlock() directly.
 */

#ifndef GARRY_WIN32_THREADING_IMPL_H
#define GARRY_WIN32_THREADING_IMPL_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef CRITICAL_SECTION garry_mutex;

typedef struct {
    SRWLOCK lock;
} garry_rwlock;

#endif
