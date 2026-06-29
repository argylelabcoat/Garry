/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file posix/threading.c
 * @brief POSIX threading implementation using pure pthreads.
 */

#include "platform_src/platform_threading.h"
#include "platform_src/posix/threading_impl.h"
#include <pthread.h>

garry_bool garry_mutex_init(void* m) {
    return pthread_mutex_init((pthread_mutex_t*)m, NULL) == 0 ? 1 : 0;
}

void garry_mutex_lock(void* m) {
    pthread_mutex_lock((pthread_mutex_t*)m);
}

void garry_mutex_unlock(void* m) {
    pthread_mutex_unlock((pthread_mutex_t*)m);
}

void garry_mutex_destroy(void* m) {
    pthread_mutex_destroy((pthread_mutex_t*)m);
}

garry_bool garry_rwlock_init(void* rw) {
    return pthread_rwlock_init(&((garry_rwlock*)rw)->rw, NULL) == 0 ? 1 : 0;
}

void garry_rwlock_rdlock(void* rw) {
    pthread_rwlock_rdlock(&((garry_rwlock*)rw)->rw);
}

void garry_rwlock_wrlock(void* rw) {
    pthread_rwlock_wrlock(&((garry_rwlock*)rw)->rw);
}

void garry_rwlock_unlock(void* rw) {
    pthread_rwlock_unlock(&((garry_rwlock*)rw)->rw);
}

void garry_rwlock_rdunlock(void* rw) {
    pthread_rwlock_unlock(&((garry_rwlock*)rw)->rw);
}

void garry_rwlock_wrunlock(void* rw) {
    pthread_rwlock_unlock(&((garry_rwlock*)rw)->rw);
}

void garry_rwlock_destroy(void* rw) {
    pthread_rwlock_destroy(&((garry_rwlock*)rw)->rw);
}
