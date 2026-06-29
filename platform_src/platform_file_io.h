/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file platform_file_io.h
 * @brief Common file I/O interface for all platforms.
 *
 * Each platform implements the functions declared here.
 * The struct garry_file_descriptor is defined identically across platforms;
 * only the underlying syscalls differ.
 */

#ifndef GARRY_PLATFORM_FILE_IO_H
#define GARRY_PLATFORM_FILE_IO_H

#include "garry/types.h"

#ifndef GARRY_O_RDONLY
#define GARRY_O_RDONLY 0
#endif
#ifndef GARRY_O_WRONLY
#define GARRY_O_WRONLY 1
#endif
#ifndef GARRY_O_RDWR
#define GARRY_O_RDWR   2
#endif
#ifndef GARRY_O_CREAT
#define GARRY_O_CREAT  0100
#endif
#ifndef GARRY_O_TRUNC
#define GARRY_O_TRUNC  01000
#endif

typedef struct {
#ifdef _WIN32
    void* fd;  /* Win32 HANDLE — 64-bit on x86_64, can't use int */
#else
    int fd;    /* POSIX file descriptor */
#endif
    const char* path;
    garry_bool is_open;
} garry_file_descriptor;

garry_file_descriptor garry_file_open(const char* path, int flags);
void garry_file_read_page(garry_file_descriptor* fd, garry_i32 page_num,
                          garry_byte* buf, garry_i32 page_size);
garry_bool garry_file_write_page(garry_file_descriptor* fd, garry_i32 page_num,
                                const garry_byte* buf, garry_i32 page_size);
garry_i32 garry_file_read(garry_file_descriptor* fd, garry_byte* buf, garry_i32 count);
void garry_file_sync(garry_file_descriptor* fd);
void garry_file_close(garry_file_descriptor* fd);
garry_i32 garry_file_unlink(const char* path);
garry_i32 garry_file_seek(garry_file_descriptor* fd, garry_i32 offset, int whence);
garry_i32 garry_file_write_bytes(garry_file_descriptor* fd, const garry_byte* buf, garry_i32 count);
garry_bool garry_file_truncate(garry_file_descriptor* fd, garry_i32 size);

#endif
