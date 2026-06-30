using System;
using NUnit.Framework;

namespace MeowDB.Tests
{
    [TestFixture]
    public class KeyEncoderTests
    {
        [Test]
        public void Encode_SinglePart_ReturnsLengthPrefixedBytes()
        {
            var result = KeyEncoder.Encode("Alice");
            Assert.AreEqual(6, result.Length);
            Assert.AreEqual(5, result[0]);
            Assert.AreEqual((byte)'A', result[1]);
        }

        [Test]
        public void Encode_MultipleParts_ReturnsConcatenatedLengthPrefixed()
        {
            var result = KeyEncoder.Encode("Alice", "Smith");
            Assert.AreEqual(12, result.Length);
            Assert.AreEqual(5, result[0]);
            Assert.AreEqual(5, result[6]);
        }

        [Test]
        public void Encode_EmptyString_ReturnsZeroLengthPrefix()
        {
            var result = KeyEncoder.Encode("");
            Assert.AreEqual(1, result.Length);
            Assert.AreEqual(0, result[0]);
        }

        [Test]
        public void Decode_SinglePart_ReturnsOriginalString()
        {
            var encoded = KeyEncoder.Encode("Alice");
            var result = KeyEncoder.DecodeSingle(encoded);
            Assert.AreEqual("Alice", result);
        }

        [Test]
        public void Decode_MultipleParts_ReturnsAllParts()
        {
            var encoded = KeyEncoder.Encode("Alice", "Smith");
            var result = KeyEncoder.DecodeParts(encoded);
            Assert.AreEqual(new[] { "Alice", "Smith" }, result);
        }

        [Test]
        public void RoundTrip_ComplexKey_ReturnsOriginal()
        {
            var parts = new[] { "users", "alice", "address", "city" };
            var encoded = KeyEncoder.Encode(parts);
            var decoded = KeyEncoder.DecodeParts(encoded);
            Assert.AreEqual(parts, decoded);
        }

        [Test]
        public void Encode_NullPart_ThrowsArgumentNullException()
        {
            Assert.Throws<ArgumentNullException>(() => KeyEncoder.Encode("a", null));
        }

        [Test]
        public void Encode_NoParts_ReturnsEmptyArray()
        {
            var result = KeyEncoder.Encode();
            Assert.AreEqual(0, result.Length);
        }

        [Test]
        public void DecodeParts_EmptyArray_ReturnsEmptyList()
        {
            var result = KeyEncoder.DecodeParts(new byte[0]);
            Assert.AreEqual(0, result.Length);
        }
    }
}
