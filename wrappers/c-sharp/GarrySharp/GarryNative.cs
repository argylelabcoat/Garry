// GarrySharp/GarryNative.cs
using System;
using System.Runtime.InteropServices;

namespace GarrySharp;

/// <summary>
/// Raw P/Invoke bindings for the Garry C library.
/// </summary>
internal static class GarryNative
{
    private const string LibName = "garry";

    // Status codes (match C enum in types.h)
    public const int GARRY_OK = 0;
    public const int GARRY_ERR_NOT_FOUND = 1;
    public const int GARRY_ERR_LOCK_CONFLICT = 2;
    public const int GARRY_ERR_IO = 3;
    public const int GARRY_ERR_CORRUPT = 4;
    public const int GARRY_ERR_INVALID_ARG = 5;
    public const int GARRY_ERR_NOMEM = 6;
    public const int GARRY_ERR_BUFFER_TOO_SMALL = 7;

    // Data inspection results (match config.h)
    public const int GARRY_DATA_NOT_FOUND = 0;
    public const int GARRY_DATA_HAS_CHILDREN = 1;
    public const int GARRY_DATA_HAS_VALUE = 10;
    public const int GARRY_DATA_HAS_BOTH = 11;

    // Database lifecycle
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr garry_database_create([MarshalAs(UnmanagedType.LPStr)] string path);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr garry_database_create_with_config(
        [MarshalAs(UnmanagedType.LPStr)] string path,
        GarryConfigNative config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr garry_database_open([MarshalAs(UnmanagedType.LPStr)] string path);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void garry_database_close(IntPtr db);

    // Transactions
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int garry_txn_begin(IntPtr db);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void garry_txn_commit(IntPtr db, int txn);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void garry_txn_rollback(IntPtr db, int txn);

    // CRUD operations
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int garry_get(
        IntPtr db, int txn,
        byte[] key, int klen,
        byte[] value, ref int vlen);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int garry_set(
        IntPtr db, int txn,
        byte[] key, int klen,
        byte[] value, int vlen);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int garry_delete(
        IntPtr db, int txn,
        byte[] key, int klen);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int garry_data(
        IntPtr db, int txn,
        byte[] key, int klen);

    // Key encoding
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int garry_key_split(
        [MarshalAs(UnmanagedType.LPStr)] string str,
        char delimiter,
        byte[] outBuf);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int garry_make_key_parts(
        [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] parts,
        int count,
        byte[] outBuf);

    // Cursor
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr garry_cursor_open(
        IntPtr db, int txn,
        byte[] prefix, int plen);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int garry_cursor_next(
        IntPtr cur,
        byte[] key, ref int klen,
        byte[] value, ref int vlen);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void garry_cursor_close(IntPtr cur);

    // String convenience (from garry_string.h)
    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int garry_set_str(
        IntPtr db, int txn,
        [MarshalAs(UnmanagedType.LPStr)] string key,
        [MarshalAs(UnmanagedType.LPStr)] string value);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int garry_get_str(
        IntPtr db, int txn,
        [MarshalAs(UnmanagedType.LPStr)] string key,
        byte[] value, int valueSize);
}
