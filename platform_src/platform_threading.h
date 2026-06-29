/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file platform_threading.h
 * @brief Common threading interface for mutex and reader-writer locks.
 *
 * Each platform defines the concrete garry_mutex and garry_rwlock types
 * via the matching platform header (posix/threading.h, win32/threading.h, ...).
 */

#ifndef GARRY_PLATFORM_THREADING_H
#define GARRY_PLATFORM_THREADING_H

#include "garry/types.h"

garry_bool garry_mutex_init(void* m);
void garry_mutex_lock(void* m);
void garry_mutex_unlock(void* m);
void garry_mutex_destroy(void* m);

garry_bool garry_rwlock_init(void* rw);
void garry_rwlock_rdlock(void* rw);
void garry_rwlock_wrlock(void* rw);
void garry_rwlock_rdunlock(void* rw);
void garry_rwlock_wrunlock(void* rw);
void garry_rwlock_unlock(void* rw);
void garry_rwlock_destroy(void* rw);

#endif
