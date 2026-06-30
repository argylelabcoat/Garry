using System;
using GarrySharp;
using Xunit;

namespace GarrySharp.Tests;

public class KeyEncoderTests
{
    [Fact]
    public void Encode_SinglePart_ReturnsLengthPrefixedBytes()
    {
        var result = KeyEncoder.Encode("Alice");
        Assert.Equal(6, result.Length);
        Assert.Equal(5, result[0]);
        Assert.Equal((byte)'A', result[1]);
    }

    [Fact]
    public void Encode_MultipleParts_ReturnsConcatenatedLengthPrefixed()
    {
        var result = KeyEncoder.Encode("Alice", "Smith");
        Assert.Equal(12, result.Length);
        Assert.Equal(5, result[0]);
        Assert.Equal(5, result[6]);
    }

    [Fact]
    public void Encode_EmptyString_ReturnsZeroLengthPrefix()
    {
        var result = KeyEncoder.Encode("");
        Assert.Equal(1, result.Length);
        Assert.Equal(0, result[0]);
    }

    [Fact]
    public void Decode_SinglePart_ReturnsOriginalString()
    {
        var encoded = KeyEncoder.Encode("Alice");
        var result = KeyEncoder.DecodeSingle(encoded);
        Assert.Equal("Alice", result);
    }

    [Fact]
    public void Decode_MultipleParts_ReturnsAllParts()
    {
        var encoded = KeyEncoder.Encode("Alice", "Smith");
        var result = KeyEncoder.DecodeParts(encoded);
        Assert.Equal(new[] { "Alice", "Smith" }, result);
    }

    [Fact]
    public void RoundTrip_ComplexKey_ReturnsOriginal()
    {
        var parts = new[] { "users", "alice", "address", "city" };
        var encoded = KeyEncoder.Encode(parts);
        var decoded = KeyEncoder.DecodeParts(encoded);
        Assert.Equal(parts, decoded);
    }

    [Fact]
    public void Encode_NullPart_ThrowsArgumentNullException()
    {
        Assert.Throws<ArgumentNullException>(() => KeyEncoder.Encode("a", null!));
    }

    [Fact]
    public void Encode_NoParts_ReturnsEmptyArray()
    {
        var result = KeyEncoder.Encode();
        Assert.Empty(result);
    }

    [Fact]
    public void DecodeParts_EmptyArray_ReturnsEmptyList()
    {
        var result = KeyEncoder.DecodeParts(Array.Empty<byte>());
        Assert.Empty(result);
    }
}
