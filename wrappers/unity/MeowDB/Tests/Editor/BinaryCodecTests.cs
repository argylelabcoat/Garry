using System;
using System.Collections.Generic;
using NUnit.Framework;

namespace MeowDB.Tests
{
    [TestFixture]
    public class BinaryCodecTests
    {
        [TestCase(null, new byte[] { 0x00 })]
        [TestCase(true, new byte[] { 0x01, 0x01 })]
        [TestCase(false, new byte[] { 0x01, 0x00 })]
        public void Encode_Primitive_ReturnsCorrectBytes(object value, byte[] expected)
        {
            var result = BinaryCodec.Encode(value);
            Assert.AreEqual(expected, result);
        }

        [Test]
        public void Encode_Int32_ReturnsTagPlus4BytesLE()
        {
            var result = BinaryCodec.Encode(42);
            Assert.AreEqual(new byte[] { 0x06, 42, 0, 0, 0 }, result);
        }

        [Test]
        public void Encode_String_ReturnsTagPlusUtf8Bytes()
        {
            var result = BinaryCodec.Encode("Hello");
            Assert.AreEqual(new byte[] { 0x0C, (byte)'H', (byte)'e', (byte)'l', (byte)'l', (byte)'o' }, result);
        }

        [Test]
        public void Encode_NullableNull_ReturnsNullTag()
        {
            string value = null;
            var result = BinaryCodec.Encode(value);
            Assert.AreEqual(new byte[] { 0x00 }, result);
        }

        [Test]
        public void Encode_Byte_ReturnsTagPlusSingleByte()
        {
            var result = BinaryCodec.Encode((byte)0xAB);
            Assert.AreEqual(new byte[] { 0x02, 0xAB }, result);
        }

        [Test]
        public void Encode_Int64_ReturnsTagPlus8BytesLE()
        {
            var result = BinaryCodec.Encode(123456789L);
            var expected = new List<byte> { 0x08 };
            expected.AddRange(BitConverter.GetBytes(123456789L));
            Assert.AreEqual(expected.ToArray(), result);
        }

        [Test]
        public void Encode_Single_ReturnsTagPlus4BytesIEEE754()
        {
            var result = BinaryCodec.Encode(3.14f);
            var expected = new List<byte> { 0x0A };
            expected.AddRange(BitConverter.GetBytes(3.14f));
            Assert.AreEqual(expected.ToArray(), result);
        }

        [Test]
        public void Encode_Double_ReturnsTagPlus8BytesIEEE754()
        {
            var result = BinaryCodec.Encode(2.718281828);
            var expected = new List<byte> { 0x0B };
            expected.AddRange(BitConverter.GetBytes(2.718281828));
            Assert.AreEqual(expected.ToArray(), result);
        }

        [Test]
        public void Encode_ByteArray_ReturnsTagPlusLengthPlusRaw()
        {
            var input = new byte[] { 1, 2, 3, 4 };
            var result = BinaryCodec.Encode(input);
            Assert.AreEqual(new byte[] { 0x0D, 4, 0, 0, 0, 1, 2, 3, 4 }, result);
        }

        [Test]
        public void Encode_Guid_ReturnsTagPlus16Bytes()
        {
            var guid = Guid.NewGuid();
            var result = BinaryCodec.Encode(guid);
            Assert.AreEqual(17, result.Length);
            Assert.AreEqual(0x0F, result[0]);
        }

        [Test]
        public void Encode_SByte_ReturnsTagPlusSingleByte()
        {
            var result = BinaryCodec.Encode((sbyte)-42);
            Assert.AreEqual(new byte[] { 0x03, unchecked((byte)-42) }, result);
        }

        [Test]
        public void Encode_Int16_ReturnsTagPlus2BytesLE()
        {
            var result = BinaryCodec.Encode((short)1234);
            Assert.AreEqual(new byte[] { 0x04, 0xD2, 0x04 }, result);
        }

        [Test]
        public void Encode_UInt16_ReturnsTagPlus2BytesLE()
        {
            var result = BinaryCodec.Encode((ushort)54321);
            Assert.AreEqual(new byte[] { 0x05, 0x31, 0xD4 }, result);
        }

        [Test]
        public void Encode_UInt32_ReturnsTagPlus4BytesLE()
        {
            var result = BinaryCodec.Encode((uint)3000000000);
            var expected = new List<byte> { 0x07 };
            expected.AddRange(BitConverter.GetBytes((uint)3000000000));
            Assert.AreEqual(expected.ToArray(), result);
        }

        [Test]
        public void Encode_UInt64_ReturnsTagPlus8BytesLE()
        {
            var result = BinaryCodec.Encode((ulong)7000000000);
            var expected = new List<byte> { 0x09 };
            expected.AddRange(BitConverter.GetBytes((ulong)7000000000));
            Assert.AreEqual(expected.ToArray(), result);
        }

        [Test]
        public void Encode_DateTime_ReturnsTagPlus8BytesTicks()
        {
            var dt = new DateTime(2024, 1, 15, 10, 30, 0, DateTimeKind.Utc);
            var result = BinaryCodec.Encode(dt);
            var expected = new List<byte> { 0x0E };
            expected.AddRange(BitConverter.GetBytes(dt.Ticks));
            Assert.AreEqual(expected.ToArray(), result);
        }

        [Test]
        public void Encode_EmptyString_ReturnsTagOnly()
        {
            var result = BinaryCodec.Encode("");
            Assert.AreEqual(new byte[] { 0x0C }, result);
        }

        [Test]
        public void Encode_EmptyByteArray_ReturnsTagPlusZeroLength()
        {
            var result = BinaryCodec.Encode(new byte[0]);
            Assert.AreEqual(new byte[] { 0x0D, 0, 0, 0, 0 }, result);
        }

        [TestCase(int.MinValue)]
        [TestCase(int.MaxValue)]
        public void Encode_Int32_BoundaryValues(int value)
        {
            var result = BinaryCodec.Encode(value);
            var expected = new List<byte> { 0x06 };
            expected.AddRange(BitConverter.GetBytes(value));
            Assert.AreEqual(expected.ToArray(), result);
        }

        [Test]
        public void Encode_Double_NaN_ReturnsTagPlusNaNBytes()
        {
            var result = BinaryCodec.Encode(double.NaN);
            var expected = new List<byte> { 0x0B };
            expected.AddRange(BitConverter.GetBytes(double.NaN));
            Assert.AreEqual(expected.ToArray(), result);
        }

        [Test]
        public void Decode_NullTag_ReturnsNull()
        {
            var result = BinaryCodec.Decode(new byte[] { 0x00 });
            Assert.IsNull(result);
        }

        [Test]
        public void Decode_BoolTrue_ReturnsTrue()
        {
            var result = BinaryCodec.Decode(new byte[] { 0x01, 0x01 });
            Assert.AreEqual(true, result);
        }

        [Test]
        public void Decode_Int32_ReturnsCorrectValue()
        {
            var result = BinaryCodec.Decode(new byte[] { 0x06, 42, 0, 0, 0 });
            Assert.AreEqual(42, result);
        }

        [Test]
        public void Decode_String_ReturnsUtf8String()
        {
            var result = BinaryCodec.Decode(new byte[] { 0x0C, (byte)'H', (byte)'i' });
            Assert.AreEqual("Hi", result);
        }

        [Test]
        public void Decode_Int64_ReturnsCorrectValue()
        {
            var bytes = new byte[9];
            bytes[0] = 0x08;
            byte[] raw = BitConverter.GetBytes(123456789L);
            if (BitConverter.IsLittleEndian)
            {
                for (int i = 0; i < 8; i++) bytes[1 + i] = raw[i];
            }
            else
            {
                for (int i = 0; i < 8; i++) bytes[1 + i] = raw[7 - i];
            }
            var result = BinaryCodec.Decode(bytes);
            Assert.AreEqual(123456789L, result);
        }

        [Test]
        public void Decode_ByteArray_RoundTrips()
        {
            var original = new byte[] { 10, 20, 30 };
            var encoded = BinaryCodec.Encode(original);
            var decoded = BinaryCodec.Decode(encoded);
            Assert.IsInstanceOf<byte[]>(decoded);
            Assert.AreEqual(original, (byte[])decoded);
        }

        [Test]
        public void Decode_Guid_RoundTrips()
        {
            var original = Guid.NewGuid();
            var encoded = BinaryCodec.Encode(original);
            var decoded = BinaryCodec.Decode(encoded);
            Assert.AreEqual(original, decoded);
        }

        [Test]
        public void Decode_UnknownTag_ThrowsNotSupportedException()
        {
            Assert.Throws<NotSupportedException>(() => BinaryCodec.Decode(new byte[] { 0xFF }));
        }
    }
}
