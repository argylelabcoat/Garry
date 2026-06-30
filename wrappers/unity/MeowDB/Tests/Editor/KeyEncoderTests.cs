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
            Assert.That(result.Length, Is.EqualTo(6));
            Assert.That(result[0], Is.EqualTo(5));
            Assert.That(result[1], Is.EqualTo((byte)'A'));
        }

        [Test]
        public void Encode_MultipleParts_ReturnsConcatenatedLengthPrefixed()
        {
            var result = KeyEncoder.Encode("Alice", "Smith");
            Assert.That(result.Length, Is.EqualTo(12));
            Assert.That(result[0], Is.EqualTo(5));
            Assert.That(result[6], Is.EqualTo(5));
        }

        [Test]
        public void Encode_EmptyString_ReturnsZeroLengthPrefix()
        {
            var result = KeyEncoder.Encode("");
            Assert.That(result.Length, Is.EqualTo(1));
            Assert.That(result[0], Is.EqualTo(0));
        }

        [Test]
        public void Decode_SinglePart_ReturnsOriginalString()
        {
            var encoded = KeyEncoder.Encode("Alice");
            var result = KeyEncoder.DecodeSingle(encoded);
            Assert.That(result, Is.EqualTo("Alice"));
        }

        [Test]
        public void Decode_MultipleParts_ReturnsAllParts()
        {
            var encoded = KeyEncoder.Encode("Alice", "Smith");
            var result = KeyEncoder.DecodeParts(encoded);
            Assert.That(result, Is.EqualTo(new[] { "Alice", "Smith" }));
        }

        [Test]
        public void RoundTrip_ComplexKey_ReturnsOriginal()
        {
            var parts = new[] { "users", "alice", "address", "city" };
            var encoded = KeyEncoder.Encode(parts);
            var decoded = KeyEncoder.DecodeParts(encoded);
            Assert.That(decoded, Is.EqualTo(parts));
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
            Assert.That(result.Length, Is.EqualTo(0));
        }

        [Test]
        public void DecodeParts_EmptyArray_ReturnsEmptyList()
        {
            var result = KeyEncoder.DecodeParts(new byte[0]);
            Assert.That(result.Length, Is.EqualTo(0));
        }
    }
}
