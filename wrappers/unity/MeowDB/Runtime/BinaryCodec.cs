using System;
using System.Text;

namespace MeowDB
{
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

        public static byte[] Encode(object value)
        {
            if (value == null)
                return new byte[] { TAG_NULL };

            if (value is bool b)
                return new byte[] { TAG_BOOL, (byte)(b ? 1 : 0) };
            if (value is byte bt)
                return new byte[] { TAG_BYTE, bt };
            if (value is sbyte sb)
                return new byte[] { TAG_SBYTE, (byte)sb };
            if (value is short i16)
                return WriteInt16(TAG_INT16, i16);
            if (value is ushort u16)
                return WriteUInt16(TAG_UINT16, u16);
            if (value is int i32)
                return WriteInt32(TAG_INT32, i32);
            if (value is uint u32)
                return WriteUInt32(TAG_UINT32, u32);
            if (value is long i64)
                return WriteInt64(TAG_INT64, i64);
            if (value is ulong u64)
                return WriteUInt64(TAG_UINT64, u64);
            if (value is float f)
                return WriteSingle(TAG_SINGLE, f);
            if (value is double d)
                return WriteDouble(TAG_DOUBLE, d);
            if (value is string s)
                return WriteString(TAG_STRING, s);
            if (value is byte[] bytes)
                return WriteBytes(TAG_BYTES, bytes);
            if (value is DateTime dt)
                return WriteInt64(TAG_DATETIME, dt.Ticks);
            if (value is Guid g)
                return WriteGuid(TAG_GUID, g);

            throw new NotSupportedException($"Type {value.GetType()} is not supported by BinaryCodec.");
        }

        private static byte[] WriteInt16(byte tag, short value)
        {
            var buf = new byte[3];
            buf[0] = tag;
            byte[] raw = BitConverter.GetBytes(value);
            if (BitConverter.IsLittleEndian)
            {
                buf[1] = raw[0];
                buf[2] = raw[1];
            }
            else
            {
                buf[1] = raw[1];
                buf[2] = raw[0];
            }
            return buf;
        }

        private static byte[] WriteUInt16(byte tag, ushort value)
        {
            var buf = new byte[3];
            buf[0] = tag;
            byte[] raw = BitConverter.GetBytes(value);
            if (BitConverter.IsLittleEndian)
            {
                buf[1] = raw[0];
                buf[2] = raw[1];
            }
            else
            {
                buf[1] = raw[1];
                buf[2] = raw[0];
            }
            return buf;
        }

        private static byte[] WriteInt32(byte tag, int value)
        {
            var buf = new byte[5];
            buf[0] = tag;
            byte[] raw = BitConverter.GetBytes(value);
            if (BitConverter.IsLittleEndian)
            {
                buf[1] = raw[0]; buf[2] = raw[1]; buf[3] = raw[2]; buf[4] = raw[3];
            }
            else
            {
                buf[1] = raw[3]; buf[2] = raw[2]; buf[3] = raw[1]; buf[4] = raw[0];
            }
            return buf;
        }

        private static byte[] WriteUInt32(byte tag, uint value)
        {
            var buf = new byte[5];
            buf[0] = tag;
            byte[] raw = BitConverter.GetBytes(value);
            if (BitConverter.IsLittleEndian)
            {
                buf[1] = raw[0]; buf[2] = raw[1]; buf[3] = raw[2]; buf[4] = raw[3];
            }
            else
            {
                buf[1] = raw[3]; buf[2] = raw[2]; buf[3] = raw[1]; buf[4] = raw[0];
            }
            return buf;
        }

        private static byte[] WriteInt64(byte tag, long value)
        {
            var buf = new byte[9];
            buf[0] = tag;
            byte[] raw = BitConverter.GetBytes(value);
            if (BitConverter.IsLittleEndian)
            {
                for (int i = 0; i < 8; i++) buf[1 + i] = raw[i];
            }
            else
            {
                for (int i = 0; i < 8; i++) buf[1 + i] = raw[7 - i];
            }
            return buf;
        }

        private static byte[] WriteUInt64(byte tag, ulong value)
        {
            var buf = new byte[9];
            buf[0] = tag;
            byte[] raw = BitConverter.GetBytes(value);
            if (BitConverter.IsLittleEndian)
            {
                for (int i = 0; i < 8; i++) buf[1 + i] = raw[i];
            }
            else
            {
                for (int i = 0; i < 8; i++) buf[1 + i] = raw[7 - i];
            }
            return buf;
        }

        private static byte[] WriteSingle(byte tag, float value)
        {
            var buf = new byte[5];
            buf[0] = tag;
            byte[] raw = BitConverter.GetBytes(value);
            if (BitConverter.IsLittleEndian)
            {
                buf[1] = raw[0]; buf[2] = raw[1]; buf[3] = raw[2]; buf[4] = raw[3];
            }
            else
            {
                buf[1] = raw[3]; buf[2] = raw[2]; buf[3] = raw[1]; buf[4] = raw[0];
            }
            return buf;
        }

        private static byte[] WriteDouble(byte tag, double value)
        {
            var buf = new byte[9];
            buf[0] = tag;
            byte[] raw = BitConverter.GetBytes(value);
            if (BitConverter.IsLittleEndian)
            {
                for (int i = 0; i < 8; i++) buf[1 + i] = raw[i];
            }
            else
            {
                for (int i = 0; i < 8; i++) buf[1 + i] = raw[7 - i];
            }
            return buf;
        }

        private static byte[] WriteString(byte tag, string value)
        {
            byte[] bytes = Encoding.UTF8.GetBytes(value);
            var buf = new byte[1 + bytes.Length];
            buf[0] = tag;
            bytes.CopyTo(buf, 1);
            return buf;
        }

        private static byte[] WriteBytes(byte tag, byte[] value)
        {
            var buf = new byte[5 + value.Length];
            buf[0] = tag;
            byte[] lenBytes = BitConverter.GetBytes(value.Length);
            if (BitConverter.IsLittleEndian)
            {
                buf[1] = lenBytes[0]; buf[2] = lenBytes[1]; buf[3] = lenBytes[2]; buf[4] = lenBytes[3];
            }
            else
            {
                buf[1] = lenBytes[3]; buf[2] = lenBytes[2]; buf[3] = lenBytes[1]; buf[4] = lenBytes[0];
            }
            value.CopyTo(buf, 5);
            return buf;
        }

        private static byte[] WriteGuid(byte tag, Guid value)
        {
            var buf = new byte[17];
            buf[0] = tag;
            byte[] raw = value.ToByteArray();
            // Guid.ToByteArray() is already in mixed endian, reverse for LE
            Array.Reverse(buf, 1, 16);
            Buffer.BlockCopy(raw, 0, buf, 1, 16);
            return buf;
        }

        public static object Decode(byte[] data)
        {
            if (data == null || data.Length == 0)
                throw new ArgumentException("Data cannot be null or empty.", nameof(data));

            byte tag = data[0];

            switch (tag)
            {
                case TAG_NULL:
                    return null;
                case TAG_BOOL:
                    return data[1] != 0;
                case TAG_BYTE:
                    return data[1];
                case TAG_SBYTE:
                    return (sbyte)data[1];
                case TAG_INT16:
                    return ReadInt16LE(data, 1);
                case TAG_UINT16:
                    return ReadUInt16LE(data, 1);
                case TAG_INT32:
                    return ReadInt32LE(data, 1);
                case TAG_UINT32:
                    return ReadUInt32LE(data, 1);
                case TAG_INT64:
                    return ReadInt64LE(data, 1);
                case TAG_UINT64:
                    return ReadUInt64LE(data, 1);
                case TAG_SINGLE:
                    return ReadSingleLE(data, 1);
                case TAG_DOUBLE:
                    return ReadDoubleLE(data, 1);
                case TAG_STRING:
                    return Encoding.UTF8.GetString(data, 1, data.Length - 1);
                case TAG_BYTES:
                    return DecodeByteArray(data);
                case TAG_DATETIME:
                    return new DateTime(ReadInt64LE(data, 1), DateTimeKind.Utc);
                case TAG_GUID:
                    return DecodeGuid(data);
                default:
                    throw new NotSupportedException($"Tag 0x{tag:X2} is not supported by BinaryCodec.");
            }
        }

        private static short ReadInt16LE(byte[] buf, int offset)
        {
            if (BitConverter.IsLittleEndian)
                return BitConverter.ToInt16(buf, offset);
            byte[] raw = new byte[2];
            raw[0] = buf[offset + 1];
            raw[1] = buf[offset];
            return BitConverter.ToInt16(raw, 0);
        }

        private static ushort ReadUInt16LE(byte[] buf, int offset)
        {
            if (BitConverter.IsLittleEndian)
                return BitConverter.ToUInt16(buf, offset);
            byte[] raw = new byte[2];
            raw[0] = buf[offset + 1];
            raw[1] = buf[offset];
            return BitConverter.ToUInt16(raw, 0);
        }

        private static int ReadInt32LE(byte[] buf, int offset)
        {
            if (BitConverter.IsLittleEndian)
                return BitConverter.ToInt32(buf, offset);
            byte[] raw = new byte[4];
            for (int i = 0; i < 4; i++) raw[i] = buf[offset + 3 - i];
            return BitConverter.ToInt32(raw, 0);
        }

        private static uint ReadUInt32LE(byte[] buf, int offset)
        {
            if (BitConverter.IsLittleEndian)
                return BitConverter.ToUInt32(buf, offset);
            byte[] raw = new byte[4];
            for (int i = 0; i < 4; i++) raw[i] = buf[offset + 3 - i];
            return BitConverter.ToUInt32(raw, 0);
        }

        private static long ReadInt64LE(byte[] buf, int offset)
        {
            if (BitConverter.IsLittleEndian)
                return BitConverter.ToInt64(buf, offset);
            byte[] raw = new byte[8];
            for (int i = 0; i < 8; i++) raw[i] = buf[offset + 7 - i];
            return BitConverter.ToInt64(raw, 0);
        }

        private static ulong ReadUInt64LE(byte[] buf, int offset)
        {
            if (BitConverter.IsLittleEndian)
                return BitConverter.ToUInt64(buf, offset);
            byte[] raw = new byte[8];
            for (int i = 0; i < 8; i++) raw[i] = buf[offset + 7 - i];
            return BitConverter.ToUInt64(raw, 0);
        }

        private static float ReadSingleLE(byte[] buf, int offset)
        {
            if (BitConverter.IsLittleEndian)
                return BitConverter.ToSingle(buf, offset);
            byte[] raw = new byte[4];
            for (int i = 0; i < 4; i++) raw[i] = buf[offset + 3 - i];
            return BitConverter.ToSingle(raw, 0);
        }

        private static double ReadDoubleLE(byte[] buf, int offset)
        {
            if (BitConverter.IsLittleEndian)
                return BitConverter.ToDouble(buf, offset);
            byte[] raw = new byte[8];
            for (int i = 0; i < 8; i++) raw[i] = buf[offset + 7 - i];
            return BitConverter.ToDouble(raw, 0);
        }

        private static byte[] DecodeByteArray(byte[] data)
        {
            int length = ReadInt32LE(data, 1);
            byte[] result = new byte[length];
            Buffer.BlockCopy(data, 5, result, 0, length);
            return result;
        }

        private static Guid DecodeGuid(byte[] data)
        {
            byte[] raw = new byte[16];
            Buffer.BlockCopy(data, 1, raw, 0, 16);
            Array.Reverse(raw);
            return new Guid(raw);
        }
    }
}
