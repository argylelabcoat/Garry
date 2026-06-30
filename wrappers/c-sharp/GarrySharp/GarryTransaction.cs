using System;
using System.Collections.Generic;

namespace GarrySharp;

/// <summary>
/// Wraps a Garry transaction with IDisposable pattern.
/// Auto-rollbacks on dispose if not committed.
/// </summary>
public sealed class GarryTransaction : IDisposable
{
    private readonly IntPtr _db;
    private int _txn;
    private bool _committed;
    private bool _disposed;

    internal int Handle => _txn;

    internal GarryTransaction(IntPtr db)
    {
        _db = db;
        _txn = GarryNative.garry_txn_begin(db);
    }

    /// <summary>
    /// Get a raw value by key.
    /// </summary>
    public byte[]? Get(string key)
    {
        ThrowIfDisposed();
        var keyBytes = KeyEncoder.Encode(key);
        // First call to get size
        int vlen = 0;
        int status = GarryNative.garry_get(_db, _txn, keyBytes, keyBytes.Length, null!, ref vlen);
        if (status == GarryNative.GARRY_ERR_NOT_FOUND) return null;
        GarryException.ThrowIfError(status);

        // Allocate and read
        var value = new byte[vlen];
        status = GarryNative.garry_get(_db, _txn, keyBytes, keyBytes.Length, value, ref vlen);
        GarryException.ThrowIfError(status);
        return value;
    }

    /// <summary>
    /// Set a raw value.
    /// </summary>
    public void Set(string key, byte[] value)
    {
        ThrowIfDisposed();
        var keyBytes = KeyEncoder.Encode(key);
        int status = GarryNative.garry_set(_db, _txn, keyBytes, keyBytes.Length, value, value.Length);
        GarryException.ThrowIfError(status);
    }

    /// <summary>
    /// Delete a key.
    /// </summary>
    public bool Delete(string key)
    {
        ThrowIfDisposed();
        var keyBytes = KeyEncoder.Encode(key);
        int status = GarryNative.garry_delete(_db, _txn, keyBytes, keyBytes.Length);
        if (status == GarryNative.GARRY_ERR_NOT_FOUND) return false;
        GarryException.ThrowIfError(status);
        return true;
    }

    /// <summary>
    /// Check if a key exists.
    /// </summary>
    public bool Exists(string key)
    {
        ThrowIfDisposed();
        var keyBytes = KeyEncoder.Encode(key);
        int status = GarryNative.garry_data(_db, _txn, keyBytes, keyBytes.Length);
        return status != GarryNative.GARRY_DATA_NOT_FOUND;
    }

    /// <summary>
    /// Get an object by key.
    /// </summary>
    public T? Get<T>(string key) where T : class, new()
    {
        ThrowIfDisposed();
        // Fetch all pairs under this prefix
        var prefixBytes = KeyEncoder.Encode(key);
        var cursor = GarryNative.garry_cursor_open(_db, _txn, prefixBytes, prefixBytes.Length);
        if (cursor == IntPtr.Zero) return null;

        try
        {
            var pairs = new List<(byte[] Key, byte[] Value)>();
            var keyBuf = new byte[256];
            var valueBuf = new byte[16384];

            while (true)
            {
                int klen = keyBuf.Length;
                int vlen = valueBuf.Length;
                int result = GarryNative.garry_cursor_next(cursor, keyBuf, ref klen, valueBuf, ref vlen);
                if (result == 0) break; // exhausted

                var keyCopy = new byte[klen];
                Array.Copy(keyBuf, keyCopy, klen);
                var valueCopy = new byte[vlen];
                Array.Copy(valueBuf, valueCopy, vlen);
                pairs.Add((keyCopy, valueCopy));
            }

            return GarrySerializer.Deserialize<T>(key, pairs);
        }
        finally
        {
            GarryNative.garry_cursor_close(cursor);
        }
    }

    /// <summary>
    /// Set an object at the given key.
    /// </summary>
    public void Set<T>(string key, T obj) where T : class
    {
        ThrowIfDisposed();
        var pairs = GarrySerializer.Serialize(key, obj);
        foreach (var (keyBytes, valueBytes) in pairs)
        {
            int status = GarryNative.garry_set(_db, _txn, keyBytes, keyBytes.Length, valueBytes, valueBytes.Length);
            GarryException.ThrowIfError(status);
        }
    }

    /// <summary>
    /// Commit the transaction.
    /// </summary>
    public void Commit()
    {
        ThrowIfDisposed();
        GarryNative.garry_txn_commit(_db, _txn);
        _committed = true;
    }

    /// <summary>
    /// Rollback the transaction.
    /// </summary>
    public void Rollback()
    {
        ThrowIfDisposed();
        GarryNative.garry_txn_rollback(_db, _txn);
        _committed = true; // prevent double-rollback in Dispose
    }

    public void Dispose()
    {
        if (_disposed) return;
        if (!_committed)
        {
            GarryNative.garry_txn_rollback(_db, _txn);
        }
        _disposed = true;
    }

    private void ThrowIfDisposed()
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(GarryTransaction));
    }
}
