using System;
using System.Runtime.InteropServices;

namespace GarrySharp;

/// <summary>
/// Runtime configuration for a Garry database.
/// </summary>
public sealed class GarryConfig
{
    public int PoolSize { get; set; } = 8;
    public int MaxRecordSize { get; set; } = 16384;
    public int PageSize { get; set; } = 4096;
    public int MaxTxns { get; set; } = 4;
    public int MaxVersions { get; set; } = 64;
    public int MaxKeySize { get; set; } = 256;
    public int MaxSubscripts { get; set; } = 16;
    public bool UseCompression { get; set; } = false;
    public int BTreeFlags { get; set; } = 0;

    /// <summary>
    /// Convert to the native garry_config struct for P/Invoke.
    /// </summary>
    internal GarryConfigNative ToNative()
    {
        return new GarryConfigNative
        {
            pool_size = PoolSize,
            max_record_size = MaxRecordSize,
            page_size = PageSize,
            max_txns = MaxTxns,
            max_versions = MaxVersions,
            max_key_size = MaxKeySize,
            max_subscripts = MaxSubscripts,
            compression = UseCompression ? 1 : 0,
            btree_flags = BTreeFlags
        };
    }
}

/// <summary>
/// Native struct matching the C garry_config layout.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
internal struct GarryConfigNative
{
    public int pool_size;
    public int max_record_size;
    public int page_size;
    public int max_txns;
    public int max_versions;
    public int max_key_size;
    public int max_subscripts;
    public int compression;
    public int btree_flags;
}
