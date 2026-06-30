using System;
using System.Collections.Generic;
using System.Text;

namespace GarrySharp;

public static class KeyEncoder
{
    public static byte[] Encode(params string[] parts)
    {
        if (parts is null)
            throw new ArgumentNullException(nameof(parts));

        var result = new List<byte>();
        foreach (var part in parts)
        {
            if (part is null)
                throw new ArgumentNullException(nameof(part), "Key part cannot be null.");

            var bytes = Encoding.UTF8.GetBytes(part);
            if (bytes.Length > 255)
                throw new ArgumentException($"Key part '{part}' exceeds 255 bytes.");

            result.Add((byte)bytes.Length);
            result.AddRange(bytes);
        }
        return result.ToArray();
    }

    public static string DecodeSingle(byte[] key)
    {
        if (key is null || key.Length == 0)
            throw new ArgumentException("Key cannot be null or empty.", nameof(key));

        int length = key[0];
        if (1 + length > key.Length)
            throw new ArgumentException("Key data is truncated or corrupt.");
        return Encoding.UTF8.GetString(key, 1, length);
    }

    public static string[] DecodeParts(byte[] key)
    {
        if (key is null || key.Length == 0)
            return Array.Empty<string>();

        var parts = new List<string>();
        int offset = 0;

        while (offset < key.Length)
        {
            int length = key[offset];
            offset++;
            if (offset + length > key.Length)
                throw new ArgumentException("Key data is truncated or corrupt.");
            parts.Add(Encoding.UTF8.GetString(key, offset, length));
            offset += length;
        }

        return parts.ToArray();
    }

    public static byte[] Combine(byte[] prefix, params string[] additionalParts)
    {
        var prefixParts = DecodeParts(prefix);
        var allParts = new string[prefixParts.Length + additionalParts.Length];
        prefixParts.CopyTo(allParts, 0);
        additionalParts.CopyTo(allParts, prefixParts.Length);
        return Encode(allParts);
    }
}
