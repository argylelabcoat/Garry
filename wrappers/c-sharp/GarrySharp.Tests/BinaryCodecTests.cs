using System;
using System.Collections.Generic;
using GarrySharp;
using Xunit;

namespace GarrySharp.Tests;

public class BinaryCodecTests
{
    [Theory]
    [InlineData(null, new byte[] { 0x00 })]
    [InlineData(true, new byte[] { 0x01, 0x01 })]
    [InlineData(false, new byte[] { 0x01, 0x00 })]
    public void Encode_Primitive_ReturnsCorrectBytes(object? value, byte[] expected)
    {
        var result = BinaryCodec.Encode(value);
        Assert.Equal(expected, result);
    }

    [Fact]
    public void Encode_Int32_ReturnsTagPlus4BytesLE()
    {
        var result = BinaryCodec.Encode(42);
        Assert.Equal(new byte[] { 0x06, 42, 0, 0, 0 }, result);
    }

    [Fact]
    public void Encode_String_ReturnsTagPlusUtf8Bytes()
    {
        var result = BinaryCodec.Encode("Hello");
        Assert.Equal(new byte[] { 0x0C, (byte)'H', (byte)'e', (byte)'l', (byte)'l', (byte)'o' }, result);
    }

    [Fact]
    public void Encode_NullableNull_ReturnsNullTag()
    {
        string? value = null;
        var result = BinaryCodec.Encode(value);
        Assert.Equal(new byte[] { 0x00 }, result);
    }

    [Fact]
    public void Encode_Byte_ReturnsTagPlusSingleByte()
    {
        var result = BinaryCodec.Encode((byte)0xAB);
        Assert.Equal(new byte[] { 0x02, 0xAB }, result);
    }

    [Fact]
    public void Encode_Int64_ReturnsTagPlus8BytesLE()
    {
        var result = BinaryCodec.Encode(123456789L);
        var expected = new List<byte> { 0x08 };
        expected.AddRange(BitConverter.GetBytes(123456789L));
        Assert.Equal(expected.ToArray(), result);
    }

    [Fact]
    public void Encode_Single_ReturnsTagPlus4BytesIEEE754()
    {
        var result = BinaryCodec.Encode(3.14f);
        var expected = new List<byte> { 0x0A };
        expected.AddRange(BitConverter.GetBytes(3.14f));
        Assert.Equal(expected.ToArray(), result);
    }

    [Fact]
    public void Encode_Double_ReturnsTagPlus8BytesIEEE754()
    {
        var result = BinaryCodec.Encode(2.718281828);
        var expected = new List<byte> { 0x0B };
        expected.AddRange(BitConverter.GetBytes(2.718281828));
        Assert.Equal(expected.ToArray(), result);
    }

    [Fact]
    public void Encode_ByteArray_ReturnsTagPlusLengthPlusRaw()
    {
        var input = new byte[] { 1, 2, 3, 4 };
        var result = BinaryCodec.Encode(input);
        Assert.Equal(new byte[] { 0x0D, 4, 0, 0, 0, 1, 2, 3, 4 }, result);
    }

    [Fact]
    public void Encode_Guid_ReturnsTagPlus16Bytes()
    {
        var guid = Guid.NewGuid();
        var result = BinaryCodec.Encode(guid);
        Assert.Equal(17, result.Length); // 1 tag + 16 bytes
        Assert.Equal(0x0F, result[0]);
    }

    [Fact]
    public void Encode_SByte_ReturnsTagPlusSingleByte()
    {
        var result = BinaryCodec.Encode((sbyte)-42);
        Assert.Equal(new byte[] { 0x03, unchecked((byte)-42) }, result);
    }

    [Fact]
    public void Encode_Int16_ReturnsTagPlus2BytesLE()
    {
        var result = BinaryCodec.Encode((short)1234);
        Assert.Equal(new byte[] { 0x04, 0xD2, 0x04 }, result);
    }

    [Fact]
    public void Encode_UInt16_ReturnsTagPlus2BytesLE()
    {
        var result = BinaryCodec.Encode((ushort)54321);
        Assert.Equal(new byte[] { 0x05, 0x31, 0xD4 }, result);
    }

    [Fact]
    public void Encode_UInt32_ReturnsTagPlus4BytesLE()
    {
        var result = BinaryCodec.Encode((uint)3000000000);
        var expected = new List<byte> { 0x07 };
        expected.AddRange(BitConverter.GetBytes((uint)3000000000));
        Assert.Equal(expected.ToArray(), result);
    }

    [Fact]
    public void Encode_UInt64_ReturnsTagPlus8BytesLE()
    {
        var result = BinaryCodec.Encode((ulong)7000000000);
        var expected = new List<byte> { 0x09 };
        expected.AddRange(BitConverter.GetBytes((ulong)7000000000));
        Assert.Equal(expected.ToArray(), result);
    }

    [Fact]
    public void Encode_DateTime_ReturnsTagPlus8BytesTicks()
    {
        var dt = new DateTime(2024, 1, 15, 10, 30, 0, DateTimeKind.Utc);
        var result = BinaryCodec.Encode(dt);
        var expected = new List<byte> { 0x0E };
        expected.AddRange(BitConverter.GetBytes(dt.Ticks));
        Assert.Equal(expected.ToArray(), result);
    }

    [Fact]
    public void Encode_EmptyString_ReturnsTagOnly()
    {
        var result = BinaryCodec.Encode("");
        Assert.Equal(new byte[] { 0x0C }, result);
    }

    [Fact]
    public void Encode_EmptyByteArray_ReturnsTagPlusZeroLength()
    {
        var result = BinaryCodec.Encode(Array.Empty<byte>());
        Assert.Equal(new byte[] { 0x0D, 0, 0, 0, 0 }, result);
    }

    [Theory]
    [InlineData(int.MinValue)]
    [InlineData(int.MaxValue)]
    public void Encode_Int32_BoundaryValues(int value)
    {
        var result = BinaryCodec.Encode(value);
        var expected = new List<byte> { 0x06 };
        expected.AddRange(BitConverter.GetBytes(value));
        Assert.Equal(expected.ToArray(), result);
    }

    [Fact]
    public void Encode_Double_NaN_ReturnsTagPlusNaNBytes()
    {
        var result = BinaryCodec.Encode(double.NaN);
        var expected = new List<byte> { 0x0B };
        expected.AddRange(BitConverter.GetBytes(double.NaN));
        Assert.Equal(expected.ToArray(), result);
    }

    [Fact]
    public void Decode_NullTag_ReturnsNull()
    {
        var result = BinaryCodec.Decode(new byte[] { 0x00 });
        Assert.Null(result);
    }

    [Fact]
    public void Decode_BoolTrue_ReturnsTrue()
    {
        var result = BinaryCodec.Decode(new byte[] { 0x01, 0x01 });
        Assert.Equal(true, result);
    }

    [Fact]
    public void Decode_Int32_ReturnsCorrectValue()
    {
        var result = BinaryCodec.Decode(new byte[] { 0x06, 42, 0, 0, 0 });
        Assert.Equal(42, result);
    }

    [Fact]
    public void Decode_String_ReturnsUtf8String()
    {
        var result = BinaryCodec.Decode(new byte[] { 0x0C, (byte)'H', (byte)'i' });
        Assert.Equal("Hi", result);
    }

    [Fact]
    public void Decode_Int64_ReturnsCorrectValue()
    {
        var bytes = new byte[9];
        bytes[0] = 0x08;
        System.Buffers.Binary.BinaryPrimitives.WriteInt64LittleEndian(bytes.AsSpan(1), 123456789L);
        var result = BinaryCodec.Decode(bytes);
        Assert.Equal(123456789L, result);
    }

    [Fact]
    public void Decode_ByteArray_RoundTrips()
    {
        var original = new byte[] { 10, 20, 30 };
        var encoded = BinaryCodec.Encode(original);
        var decoded = BinaryCodec.Decode(encoded);
        Assert.IsType<byte[]>(decoded);
        Assert.Equal(original, (byte[])decoded!);
    }

    [Fact]
    public void Decode_Guid_RoundTrips()
    {
        var original = Guid.NewGuid();
        var encoded = BinaryCodec.Encode(original);
        var decoded = BinaryCodec.Decode(encoded);
        Assert.Equal(original, decoded);
    }

    [Fact]
    public void Decode_UnknownTag_ThrowsNotSupportedException()
    {
        Assert.Throws<NotSupportedException>(() => BinaryCodec.Decode(new byte[] { 0xFF }));
    }
}
