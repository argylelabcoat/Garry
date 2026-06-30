using System;
using System.Collections.Generic;

namespace MeowDB
{
    public sealed class MeowTransaction : IDisposable
    {
        private readonly IntPtr _db;
        private int _txn;
        private bool _committed;
        private bool _disposed;

        internal int Txn => _txn;

        internal MeowTransaction(IntPtr db)
        {
            _db = db;
            _txn = MeowNative.garry_txn_begin(db);
        }

        public byte[] Get(string key)
        {
            ThrowIfDisposed();
            byte[] keyBytes = KeyEncoder.Encode(key);
            int vlen = 0;
            int status = MeowNative.garry_get(_db, _txn, keyBytes, keyBytes.Length, null, ref vlen);
            if (status == MeowNative.GARRY_ERR_NOT_FOUND) return null;
            MeowException.ThrowIfError(status);

            var value = new byte[vlen];
            status = MeowNative.garry_get(_db, _txn, keyBytes, keyBytes.Length, value, ref vlen);
            MeowException.ThrowIfError(status);
            return value;
        }

        public void Set(string key, byte[] value)
        {
            ThrowIfDisposed();
            byte[] keyBytes = KeyEncoder.Encode(key);
            int status = MeowNative.garry_set(_db, _txn, keyBytes, keyBytes.Length, value, value.Length);
            MeowException.ThrowIfError(status);
        }

        public bool Delete(string key)
        {
            ThrowIfDisposed();
            byte[] keyBytes = KeyEncoder.Encode(key);
            int status = MeowNative.garry_delete(_db, _txn, keyBytes, keyBytes.Length);
            if (status == MeowNative.GARRY_ERR_NOT_FOUND) return false;
            MeowException.ThrowIfError(status);
            return true;
        }

        public bool Exists(string key)
        {
            ThrowIfDisposed();
            byte[] keyBytes = KeyEncoder.Encode(key);
            int status = MeowNative.garry_data(_db, _txn, keyBytes, keyBytes.Length);
            return status != MeowNative.GARRY_DATA_NOT_FOUND;
        }

        public T Get<T>(string key) where T : class, new()
        {
            ThrowIfDisposed();
            byte[] prefixBytes = KeyEncoder.Encode(key);
            IntPtr cursor = MeowNative.garry_cursor_open(_db, _txn, prefixBytes, prefixBytes.Length);
            if (cursor == IntPtr.Zero) return null;

            try
            {
                var pairs = new List<KeyValuePair<byte[], byte[]>>();
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
                    pairs.Add(new KeyValuePair<byte[], byte[]>(keyCopy, valueCopy));
                }

                return MeowSerializer.Deserialize<T>(key, pairs);
            }
            finally
            {
                MeowNative.garry_cursor_close(cursor);
            }
        }

        public void Set<T>(string key, T obj) where T : class
        {
            ThrowIfDisposed();
            var pairs = MeowSerializer.Serialize(key, obj);
            foreach (var kvp in pairs)
            {
                int status = MeowNative.garry_set(_db, _txn, kvp.Key, kvp.Key.Length, kvp.Value, kvp.Value.Length);
                MeowException.ThrowIfError(status);
            }
        }

        public void Commit()
        {
            ThrowIfDisposed();
            MeowNative.garry_txn_commit(_db, _txn);
            _committed = true;
        }

        public void Rollback()
        {
            ThrowIfDisposed();
            MeowNative.garry_txn_rollback(_db, _txn);
            _committed = true;
        }

        public void Dispose()
        {
            if (_disposed) return;
            if (!_committed)
            {
                MeowNative.garry_txn_rollback(_db, _txn);
            }
            _disposed = true;
        }

        private void ThrowIfDisposed()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(MeowTransaction));
        }
    }
}
