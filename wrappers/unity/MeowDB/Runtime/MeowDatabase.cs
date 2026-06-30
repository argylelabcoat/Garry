using System;
using System.Collections.Generic;
using System.Linq;

namespace MeowDB
{
    public sealed class MeowDatabase : IDisposable
    {
        private readonly IntPtr _handle;
        private bool _disposed;

        public MeowDatabase(string path)
        {
            _handle = MeowNative.garry_database_open(path);
            if (_handle == IntPtr.Zero)
                throw new Exception($"Failed to open MeowDB database at '{path}'.");
        }

        public MeowDatabase(string path, MeowConfig config)
        {
            _handle = MeowNative.garry_database_create_with_config(path, config.ToNative());
            if (_handle == IntPtr.Zero)
                throw new Exception($"Failed to create MeowDB database at '{path}'.");
        }

        public MeowTransaction BeginTransaction()
        {
            ThrowIfDisposed();
            return new MeowTransaction(_handle);
        }

        public byte[] Get(string key)
        {
            ThrowIfDisposed();
            using (var txn = BeginTransaction())
            {
                return txn.Get(key);
            }
        }

        public void Set(string key, byte[] value)
        {
            ThrowIfDisposed();
            using (var txn = BeginTransaction())
            {
                txn.Set(key, value);
                txn.Commit();
            }
        }

        public bool Delete(string key)
        {
            ThrowIfDisposed();
            using (var txn = BeginTransaction())
            {
                var result = txn.Delete(key);
                txn.Commit();
                return result;
            }
        }

        public bool Exists(string key)
        {
            ThrowIfDisposed();
            using (var txn = BeginTransaction())
            {
                return txn.Exists(key);
            }
        }

        public T Get<T>(string key) where T : class, new()
        {
            ThrowIfDisposed();
            using (var txn = BeginTransaction())
            {
                return txn.Get<T>(key);
            }
        }

        public void Set<T>(string key, T obj) where T : class
        {
            ThrowIfDisposed();
            using (var txn = BeginTransaction())
            {
                txn.Set(key, obj);
                txn.Commit();
            }
        }

        public IReadOnlyList<KeyValuePair<string, byte[]>> Query(string prefix)
        {
            ThrowIfDisposed();
            using (var txn = BeginTransaction())
            {
                byte[] prefixBytes = KeyEncoder.Encode(prefix);
                IntPtr cursor = MeowNative.garry_cursor_open(_handle, 0, prefixBytes, prefixBytes.Length);
                if (cursor == IntPtr.Zero) return new List<KeyValuePair<string, byte[]>>();

                try
                {
                    var results = new List<KeyValuePair<string, byte[]>>();
                    var keyBuf = new byte[256];
                    var valueBuf = new byte[16384];

                    while (true)
                    {
                        int klen = keyBuf.Length;
                        int vlen = valueBuf.Length;
                        int result = MeowNative.garry_cursor_next(cursor, keyBuf, ref klen, valueBuf, ref vlen);
                        if (result == 0) break;

                        var keyCopy = new byte[klen];
                        Array.Copy(keyBuf, keyCopy, klen);
                        var valueCopy = new byte[vlen];
                        Array.Copy(valueBuf, valueCopy, vlen);

                        string[] keyParts = KeyEncoder.DecodeParts(keyCopy);
                        results.Add(new KeyValuePair<string, byte[]>(string.Join(".", keyParts), valueCopy));
                    }

                    return results;
                }
                finally
                {
                    MeowNative.garry_cursor_close(cursor);
                }
            }
        }

        public IReadOnlyList<T> Query<T>(string prefix) where T : class, new()
        {
            ThrowIfDisposed();
            using (var txn = BeginTransaction())
            {
                byte[] prefixBytes = KeyEncoder.Encode(prefix);
                IntPtr cursor = MeowNative.garry_cursor_open(_handle, 0, prefixBytes, prefixBytes.Length);
                if (cursor == IntPtr.Zero) return new List<T>();

                try
                {
                    var results = new List<T>();
                    var keyBuf = new byte[256];
                    var valueBuf = new byte[16384];

                    var allPairs = new List<KeyValuePair<byte[], byte[]>>();
                    while (true)
                    {
                        int klen = keyBuf.Length;
                        int vlen = valueBuf.Length;
                        int result = MeowNative.garry_cursor_next(cursor, keyBuf, ref klen, valueBuf, ref vlen);
                        if (result == 0) break;

                        var keyCopy = new byte[klen];
                        Array.Copy(keyBuf, keyCopy, klen);
                        var valueCopy = new byte[vlen];
                        Array.Copy(valueBuf, valueCopy, vlen);
                        allPairs.Add(new KeyValuePair<byte[], byte[]>(keyCopy, valueCopy));
                    }

                    var groups = allPairs.GroupBy(p =>
                    {
                        string[] parts = KeyEncoder.DecodeParts(p.Key);
                        return parts.Length > 0 ? parts[0] : "";
                    });

                    foreach (var group in groups)
                    {
                        if (string.IsNullOrEmpty(group.Key)) continue;
                        var obj = MeowSerializer.Deserialize<T>(group.Key, group.ToList());
                        if (obj != null)
                            results.Add(obj);
                    }

                    return results;
                }
                finally
                {
                    MeowNative.garry_cursor_close(cursor);
                }
            }
        }

        public void Dispose()
        {
            if (_disposed) return;
            MeowNative.garry_database_close(_handle);
            _disposed = true;
        }

        private void ThrowIfDisposed()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(MeowDatabase));
        }
    }
}
