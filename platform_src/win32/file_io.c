/*
 * Copyright (c) 2026 Matthew Hughes (Argylelabcoat)
 * Licensed under the BSD-3-Clause License.
 * See LICENSE file for details.
 */

/**
 * @file win32/file_io.c
 * @brief Win32 file I/O implementation using the Windows API.
 *
 * Maps garry_file_descriptor's fd to a Win32 HANDLE directly (void*).
 * POSIX open() flags (O_RDWR, O_CREAT, O_TRUNC) are translated to
 * the corresponding CreateFileA access/share/disposition values.
 */

#include "platform_src/platform_file_io.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>

/* ---------- flag translation ---------- */

static DWORD access_from_flags(int flags)
{
    DWORD acc = 0;
    if (flags & GARRY_O_RDWR)
        acc = GENERIC_READ | GENERIC_WRITE;
    else if (flags & GARRY_O_WRONLY)
        acc = GENERIC_WRITE;
    else
        acc = GENERIC_READ;
    return acc;
}

static DWORD disposition_from_flags(int flags)
{
    if (flags & GARRY_O_CREAT) {
        if (flags & GARRY_O_TRUNC)
            return CREATE_ALWAYS;
        return OPEN_ALWAYS;
    }
    return OPEN_EXISTING;
}

/* ---------- interface implementation ---------- */

garry_file_descriptor garry_file_open(const char* path, int flags)
{
    garry_file_descriptor f;
    HANDLE h;

    f.path = path;
    f.is_open = 0;
    f.fd = NULL;

    h = CreateFileA(
        path,
        access_from_flags(flags),
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        disposition_from_flags(flags),
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (h == INVALID_HANDLE_VALUE)
        return f;

    f.fd = h;
    f.is_open = 1;
    return f;
}

void garry_file_read_page(garry_file_descriptor* fd, garry_i32 page_num,
                          garry_byte* buf, garry_i32 page_size)
{
    LARGE_INTEGER li;
    DWORD bytes_read = 0;

    li.QuadPart = (LONGLONG)page_num * (LONGLONG)page_size;
    SetFilePointerEx(fd->fd, li, NULL, FILE_BEGIN);
    ReadFile(fd->fd, buf, (DWORD)page_size, &bytes_read, NULL);
}

void garry_file_write_page(garry_file_descriptor* fd, garry_i32 page_num,
                           const garry_byte* buf, garry_i32 page_size)
{
    LARGE_INTEGER li;
    DWORD bytes_written = 0;

    li.QuadPart = (LONGLONG)page_num * (LONGLONG)page_size;
    SetFilePointerEx(fd->fd, li, NULL, FILE_BEGIN);
    WriteFile(fd->fd, buf, (DWORD)page_size, &bytes_written, NULL);
    FlushFileBuffers(fd->fd);
}

garry_i32 garry_file_read(garry_file_descriptor* fd, garry_byte* buf, garry_i32 count)
{
    DWORD bytes_read = 0;

    if (!ReadFile(fd->fd, buf, (DWORD)count, &bytes_read, NULL))
        return -1;
    return (garry_i32)bytes_read;
}

void garry_file_sync(garry_file_descriptor* fd)
{
    FlushFileBuffers(fd->fd);
}

void garry_file_close(garry_file_descriptor* fd)
{
    if (fd->is_open && fd->fd != NULL) {
        CloseHandle(fd->fd);
    }
    fd->is_open = 0;
    fd->fd = NULL;
}

garry_i32 garry_file_unlink(const char* path)
{
    return DeleteFileA(path) ? 0 : -1;
}

garry_i32 garry_file_seek(garry_file_descriptor* fd, garry_i32 offset, int whence)
{
    DWORD method;
    LARGE_INTEGER li, result;

    switch (whence) {
        case SEEK_SET: method = FILE_BEGIN;   break;
        case SEEK_CUR: method = FILE_CURRENT; break;
        case SEEK_END: method = FILE_END;     break;
        default:       return -1;
    }

    li.QuadPart = offset;
    if (!SetFilePointerEx(fd->fd, li, &result, method))
        return -1;
    return (garry_i32)result.QuadPart;
}

garry_i32 garry_file_write_bytes(garry_file_descriptor* fd, const garry_byte* buf, garry_i32 count)
{
    DWORD bytes_written = 0;

    if (!WriteFile(fd->fd, buf, (DWORD)count, &bytes_written, NULL))
        return -1;
    return (garry_i32)bytes_written;
}
