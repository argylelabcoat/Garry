// GarrySharp/GarryDatabase.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace GarrySharp;

/// <summary>
/// OOP wrapper for a Garry database. Implements IDisposable.
/// </summary>
public sealed class GarryDatabase : IDisposable
{
    private readonly IntPtr _handle;
    private bool _disposed;

    public GarryDatabase(string path)
    {
        _handle = GarryNative.garry_database_open(path);
        if (_handle == IntPtr.Zero)
            throw new IOException($"Failed to open Garry database at '{path}'.");
    }

    public GarryDatabase(string path, GarryConfig config)
    {
        _handle = GarryNative.garry_database_create_with_config(path, config.ToNative());
        if (_handle == IntPtr.Zero)
            throw new IOException($"Failed to create Garry database at '{path}'.");
    }

    /// <summary>
    /// Begin a new transaction.
    /// </summary>
    public GarryTransaction BeginTransaction()
    {
        ThrowIfDisposed();
        return new GarryTransaction(_handle);
    }

    /// <summary>
    /// Get a raw value by key (auto-transacts).
    /// </summary>
    public byte[]? Get(string key)
    {
        ThrowIfDisposed();
        using var txn = BeginTransaction();
        return txn.Get(key);
    }

    /// <summary>
    /// Set a raw value (auto-transacts).
    /// </summary>
    public void Set(string key, byte[] value)
    {
        ThrowIfDisposed();
        using var txn = BeginTransaction();
        txn.Set(key, value);
        txn.Commit();
    }

    /// <summary>
    /// Delete a key (auto-transacts).
    /// </summary>
    public bool Delete(string key)
    {
        ThrowIfDisposed();
        using var txn = BeginTransaction();
        var result = txn.Delete(key);
        txn.Commit();
        return result;
    }

    /// <summary>
    /// Check if a key exists.
    /// </summary>
    public bool Exists(string key)
    {
        ThrowIfDisposed();
        using var txn = BeginTransaction();
        return txn.Exists(key);
    }

    /// <summary>
    /// Get an object by key (auto-transacts).
    /// </summary>
    public T? Get<T>(string key) where T : class, new()
    {
        ThrowIfDisposed();
        using var txn = BeginTransaction();
        return txn.Get<T>(key);
    }

    /// <summary>
    /// Set an object (auto-transacts).
    /// </summary>
    public void Set<T>(string key, T obj) where T : class
    {
        ThrowIfDisposed();
        using var txn = BeginTransaction();
        txn.Set(key, obj);
        txn.Commit();
    }

    /// <summary>
    /// Query all key-value pairs under a prefix.
    /// </summary>
    public IReadOnlyList<(string Key, byte[] Value)> Query(string prefix)
    {
        ThrowIfDisposed();
        using var txn = BeginTransaction();
        var prefixBytes = KeyEncoder.Encode(prefix);
        var cursor = GarryNative.garry_cursor_open(_handle, txn.Handle, prefixBytes, prefixBytes.Length);
        if (cursor == IntPtr.Zero) return Array.Empty<(string, byte[])>();

        try
        {
            var results = new List<(string Key, byte[] Value)>();
            var keyBuf = new byte[256];
            var valueBuf = new byte[16384];

            while (true)
            {
                int klen = keyBuf.Length;
                int vlen = valueBuf.Length;
                int result = GarryNative.garry_cursor_next(cursor, keyBuf, ref klen, valueBuf, ref vlen);
                if (result == 0) break;

                var keyCopy = new byte[klen];
                Array.Copy(keyBuf, keyCopy, klen);
                var valueCopy = new byte[vlen];
                Array.Copy(valueBuf, valueCopy, vlen);

                var keyParts = KeyEncoder.DecodeParts(keyCopy);
                results.Add((string.Join(".", keyParts), valueCopy));
            }

            return results;
        }
        finally
        {
            GarryNative.garry_cursor_close(cursor);
        }
    }

    /// <summary>
    /// Query and deserialize all objects under a prefix.
    /// </summary>
    public IReadOnlyList<T> Query<T>(string prefix) where T : class, new()
    {
        ThrowIfDisposed();
        using var txn = BeginTransaction();
        var prefixBytes = KeyEncoder.Encode(prefix);
        var cursor = GarryNative.garry_cursor_open(_handle, txn.Handle, prefixBytes, prefixBytes.Length);
        if (cursor == IntPtr.Zero) return Array.Empty<T>();

        try
        {
            var results = new List<T>();
            var keyBuf = new byte[256];
            var valueBuf = new byte[16384];

            // Collect all pairs
            var allPairs = new List<(byte[] Key, byte[] Value)>();
            while (true)
            {
                int klen = keyBuf.Length;
                int vlen = valueBuf.Length;
                int result = GarryNative.garry_cursor_next(cursor, keyBuf, ref klen, valueBuf, ref vlen);
                if (result == 0) break;

                var keyCopy = new byte[klen];
                Array.Copy(keyBuf, keyCopy, klen);
                var valueCopy = new byte[vlen];
                Array.Copy(valueBuf, valueCopy, vlen);
                allPairs.Add((keyCopy, valueCopy));
            }

            // Group by root object key (first subscript)
            var groups = allPairs.GroupBy(p =>
            {
                var parts = KeyEncoder.DecodeParts(p.Key);
                return parts.Length > 0 ? parts[0] : "";
            });

            foreach (var group in groups)
            {
                if (string.IsNullOrEmpty(group.Key)) continue;
                var obj = GarrySerializer.Deserialize<T>(group.Key, group.ToList());
                if (obj is not null)
                    results.Add(obj);
            }

            return results;
        }
        finally
        {
            GarryNative.garry_cursor_close(cursor);
        }
    }

    public void Dispose()
    {
        if (_disposed) return;
        GarryNative.garry_database_close(_handle);
        _disposed = true;
    }

    private void ThrowIfDisposed()
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(GarryDatabase));
    }
}
