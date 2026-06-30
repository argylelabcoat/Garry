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
}
