/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file posix/file_io.c
 * @brief POSIX file I/O implementation.
 */

#include "platform_src/platform_file_io.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

garry_file_descriptor garry_file_open(const char* path, int flags)
{
    garry_file_descriptor f;
    int oflags = 0;

    if (flags & GARRY_O_RDWR) oflags |= O_RDWR;
    else if (flags & GARRY_O_WRONLY) oflags |= O_WRONLY;
    else oflags = O_RDONLY;
    if (flags & GARRY_O_CREAT) oflags |= O_CREAT;
    if (flags & GARRY_O_TRUNC) oflags |= O_TRUNC;

    f.fd = open(path, oflags, 0644);
    f.path = path;
    f.is_open = (f.fd >= 0) ? 1 : 0;
    return f;
}

void garry_file_read_page(garry_file_descriptor* fd, garry_i32 page_num,
                           garry_byte* buf, garry_i32 page_size)
{
    off_t offset = (off_t)page_num * (off_t)page_size;
    ssize_t n = pread(fd->fd, buf, (size_t)page_size, offset);
    if (n < (ssize_t)page_size) {
        memset(buf, 0, (size_t)page_size);
    }
}

void garry_file_write_page(garry_file_descriptor* fd, garry_i32 page_num,
                            const garry_byte* buf, garry_i32 page_size)
{
    off_t offset = (off_t)page_num * (off_t)page_size;
    ssize_t n = pwrite(fd->fd, buf, (size_t)page_size, offset);
    (void)n;
}

garry_i32 garry_file_read(garry_file_descriptor* fd, garry_byte* buf, garry_i32 count)
{
    ssize_t n = read(fd->fd, buf, (size_t)count);
    return (garry_i32)n;
}

void garry_file_sync(garry_file_descriptor* fd)
{
    fsync(fd->fd);
}

void garry_file_close(garry_file_descriptor* fd)
{
    if (fd->is_open && fd->fd >= 0) {
        close(fd->fd);
    }
    fd->is_open = 0;
    fd->fd = -1;
}

garry_i32 garry_file_unlink(const char* path)
{
    return (remove(path) == 0) ? 0 : -1;
}

garry_i32 garry_file_seek(garry_file_descriptor* fd, garry_i32 offset, int whence)
{
    off_t r = lseek(fd->fd, (off_t)offset, whence);
    return (r < 0) ? -1 : (garry_i32)r;
}

garry_i32 garry_file_write_bytes(garry_file_descriptor* fd, const garry_byte* buf, garry_i32 count)
{
    ssize_t n = write(fd->fd, buf, (size_t)count);
    return (garry_i32)n;
}
