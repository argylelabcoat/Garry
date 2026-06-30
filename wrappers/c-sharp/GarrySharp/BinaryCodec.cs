using System;
using System.Buffers.Binary;
using System.Text;

namespace GarrySharp;

/// <summary>
/// Encodes/decodes .NET values to/from Garry's binary format.
/// Format: [tag byte][payload]
/// </summary>
public static class BinaryCodec
{
    public const byte TAG_NULL = 0x00;
    public const byte TAG_BOOL = 0x01;
    public const byte TAG_BYTE = 0x02;
    public const byte TAG_SBYTE = 0x03;
    public const byte TAG_INT16 = 0x04;
    public const byte TAG_UINT16 = 0x05;
    public const byte TAG_INT32 = 0x06;
    public const byte TAG_UINT32 = 0x07;
    public const byte TAG_INT64 = 0x08;
    public const byte TAG_UINT64 = 0x09;
    public const byte TAG_SINGLE = 0x0A;
    public const byte TAG_DOUBLE = 0x0B;
    public const byte TAG_STRING = 0x0C;
    public const byte TAG_BYTES = 0x0D;
    public const byte TAG_DATETIME = 0x0E;
    public const byte TAG_GUID = 0x0F;

    public static byte[] Encode(object? value)
    {
        if (value is null)
            return new byte[] { TAG_NULL };

        return value switch
        {
            bool b => new byte[] { TAG_BOOL, (byte)(b ? 1 : 0) },
            byte bt => new byte[] { TAG_BYTE, bt },
            sbyte sb => new byte[] { TAG_SBYTE, (byte)sb },
            short i16 => WriteInt16(TAG_INT16, i16),
            ushort u16 => WriteUInt16(TAG_UINT16, u16),
            int i32 => WriteInt32(TAG_INT32, i32),
            uint u32 => WriteUInt32(TAG_UINT32, u32),
            long i64 => WriteInt64(TAG_INT64, i64),
            ulong u64 => WriteUInt64(TAG_UINT64, u64),
            float f => WriteSingle(TAG_SINGLE, f),
            double d => WriteDouble(TAG_DOUBLE, d),
            string s => WriteString(TAG_STRING, s),
            byte[] bytes => WriteBytes(TAG_BYTES, bytes),
            DateTime dt => WriteInt64(TAG_DATETIME, dt.Ticks),
            Guid g => WriteGuid(TAG_GUID, g),
            _ => throw new NotSupportedException($"Type {value.GetType()} is not supported by BinaryCodec.")
        };
    }

    private static byte[] WriteInt16(byte tag, short value)
    {
        var buf = new byte[3];
        buf[0] = tag;
        BinaryPrimitives.WriteInt16LittleEndian(buf.AsSpan(1), value);
        return buf;
    }

    private static byte[] WriteUInt16(byte tag, ushort value)
    {
        var buf = new byte[3];
        buf[0] = tag;
        BinaryPrimitives.WriteUInt16LittleEndian(buf.AsSpan(1), value);
        return buf;
    }

    private static byte[] WriteInt32(byte tag, int value)
    {
        var buf = new byte[5];
        buf[0] = tag;
        BinaryPrimitives.WriteInt32LittleEndian(buf.AsSpan(1), value);
        return buf;
    }

    private static byte[] WriteUInt32(byte tag, uint value)
    {
        var buf = new byte[5];
        buf[0] = tag;
        BinaryPrimitives.WriteUInt32LittleEndian(buf.AsSpan(1), value);
        return buf;
    }

    private static byte[] WriteInt64(byte tag, long value)
    {
        var buf = new byte[9];
        buf[0] = tag;
        BinaryPrimitives.WriteInt64LittleEndian(buf.AsSpan(1), value);
        return buf;
    }

    private static byte[] WriteUInt64(byte tag, ulong value)
    {
        var buf = new byte[9];
        buf[0] = tag;
        BinaryPrimitives.WriteUInt64LittleEndian(buf.AsSpan(1), value);
        return buf;
    }

    private static byte[] WriteSingle(byte tag, float value)
    {
        var buf = new byte[5];
        buf[0] = tag;
        BinaryPrimitives.WriteSingleLittleEndian(buf.AsSpan(1), value);
        return buf;
    }

    private static byte[] WriteDouble(byte tag, double value)
    {
        var buf = new byte[9];
        buf[0] = tag;
        BinaryPrimitives.WriteDoubleLittleEndian(buf.AsSpan(1), value);
        return buf;
    }

    private static byte[] WriteString(byte tag, string value)
    {
        var bytes = Encoding.UTF8.GetBytes(value);
        var buf = new byte[1 + bytes.Length];
        buf[0] = tag;
        bytes.CopyTo(buf, 1);
        return buf;
    }

    private static byte[] WriteBytes(byte tag, byte[] value)
    {
        var buf = new byte[5 + value.Length];
        buf[0] = tag;
        BinaryPrimitives.WriteInt32LittleEndian(buf.AsSpan(1), value.Length);
        value.CopyTo(buf, 5);
        return buf;
    }

    private static byte[] WriteGuid(byte tag, Guid value)
    {
        var buf = new byte[17];
        buf[0] = tag;
        value.TryWriteBytes(buf.AsSpan(1));
        return buf;
    }
}
