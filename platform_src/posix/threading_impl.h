/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file posix/threading_impl.h
 * @brief POSIX-specific type definitions for mutex and rwlock.
 */

#ifndef GARRY_POSIX_THREADING_IMPL_H
#define GARRY_POSIX_THREADING_IMPL_H

#include <pthread.h>

typedef pthread_mutex_t garry_mutex;

typedef struct {
    pthread_rwlock_t rw;
} garry_rwlock;

#endif
