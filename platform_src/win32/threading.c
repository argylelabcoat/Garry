/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file win32/threading.c
 * @brief Win32 threading implementation.
 *
 * Mutex: CRITICAL_SECTION (recursive, always available).
 * Read-write lock: SRWLOCK (Vista+). SRWLOCK is slim, non-recursive,
 * and kernel-mode backed — ideal for the single-writer / multi-reader
 * pattern Garry uses.
 */

#include "platform_src/platform_threading.h"
#include "platform_src/win32/threading_impl.h"

garry_bool garry_mutex_init(void* m) {
    InitializeCriticalSection((CRITICAL_SECTION*)m);
    return 1;
}

void garry_mutex_lock(void* m) {
    EnterCriticalSection((CRITICAL_SECTION*)m);
}

void garry_mutex_unlock(void* m) {
    LeaveCriticalSection((CRITICAL_SECTION*)m);
}

void garry_mutex_destroy(void* m) {
    DeleteCriticalSection((CRITICAL_SECTION*)m);
}

garry_bool garry_rwlock_init(void* rw) {
    garry_rwlock* r = (garry_rwlock*)rw;
    InitializeSRWLock(&r->lock);
    return 1;
}

void garry_rwlock_rdlock(void* rw) {
    garry_rwlock* r = (garry_rwlock*)rw;
    AcquireSRWLockShared(&r->lock);
}

void garry_rwlock_wrlock(void* rw) {
    garry_rwlock* r = (garry_rwlock*)rw;
    AcquireSRWLockExclusive(&r->lock);
}

void garry_rwlock_rdunlock(void* rw) {
    garry_rwlock* r = (garry_rwlock*)rw;
    ReleaseSRWLockShared(&r->lock);
}

void garry_rwlock_wrunlock(void* rw) {
    garry_rwlock* r = (garry_rwlock*)rw;
    ReleaseSRWLockExclusive(&r->lock);
}

void garry_rwlock_unlock(void* rw) {
    (void)rw;
}

void garry_rwlock_destroy(void* rw) {
    (void)rw;  /* SRWLOCK requires no cleanup */
}
