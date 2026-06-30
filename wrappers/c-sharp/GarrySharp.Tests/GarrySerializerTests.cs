using System.Collections.Generic;
using GarrySharp;
using Xunit;

namespace GarrySharp.Tests;

public class GarrySerializerTests
{
    public class Address
    {
        public string City { get; set; } = "";
        public string Zip { get; set; } = "";
    }

    public class Person
    {
        public string Name { get; set; } = "";
        public int Age { get; set; }
        public Address? Address { get; set; }
    }

    public class PersonWithIgnore
    {
        public string Name { get; set; } = "";
        [GarryIgnore]
        public string Secret { get; set; } = "";
    }

    public class PersonWithKeyAttr
    {
        [GarryKey("person_name")]
        public string Name { get; set; } = "";
    }

    [Fact]
    public void Serialize_SimpleObject_ReturnsContainerPlusLeaves()
    {
        var person = new Person { Name = "Alice", Age = 25 };
        var pairs = GarrySerializer.Serialize("Person", person);

        // Container + 2 leaves
        Assert.Equal(3, pairs.Count);

        // First pair is the container marker
        var containerKey = KeyEncoder.DecodeParts(pairs[0].Key);
        Assert.Equal("Person", containerKey[0]);

        // Check leaves
        var nameKey = KeyEncoder.DecodeParts(pairs[1].Key);
        Assert.Equal(new[] { "Person", "Name" }, nameKey);

        var ageKey = KeyEncoder.DecodeParts(pairs[2].Key);
        Assert.Equal(new[] { "Person", "Age" }, ageKey);
    }

    [Fact]
    public void Serialize_NestedObject_CreatesSubContainers()
    {
        var person = new Person
        {
            Name = "Alice",
            Age = 25,
            Address = new Address { City = "NYC", Zip = "10001" }
        };
        var pairs = GarrySerializer.Serialize("Person", person);

        // Container + Name + Age + Address container + City + Zip = 6
        Assert.Equal(6, pairs.Count);
    }

    [Fact]
    public void Serialize_IgnoresMarkedProperties()
    {
        var person = new PersonWithIgnore { Name = "Alice", Secret = "hidden" };
        var pairs = GarrySerializer.Serialize("Person", person);

        // Container + Name only (Secret is ignored)
        Assert.Equal(2, pairs.Count);
    }

    [Fact]
    public void Serialize_UsesGarryKeyName()
    {
        var person = new PersonWithKeyAttr { Name = "Alice" };
        var pairs = GarrySerializer.Serialize("Person", person);

        var nameKey = KeyEncoder.DecodeParts(pairs[1].Key);
        Assert.Equal("person_name", nameKey[1]);
    }

    [Fact]
    public void Deserialize_EmptyPairs_ReturnsDefault()
    {
        var result = GarrySerializer.Deserialize<Person>(
            "Person",
            new List<(byte[] Key, byte[] Value)>());
        Assert.NotNull(result);
        Assert.Equal("", result!.Name);
    }

    [Fact]
    public void Deserialize_WithLeafPairs_ReconstructsObject()
    {
        var pairs = new List<(byte[] Key, byte[] Value)>
        {
            (KeyEncoder.Encode("Person", "Name"), BinaryCodec.Encode("Alice")),
            (KeyEncoder.Encode("Person", "Age"), BinaryCodec.Encode(25)),
        };

        var result = GarrySerializer.Deserialize<Person>("Person", pairs);
        Assert.NotNull(result);
        Assert.Equal("Alice", result!.Name);
        Assert.Equal(25, result.Age);
    }

    [Fact]
    public void Deserialize_NestedObject_ReconstructsGraph()
    {
        var pairs = new List<(byte[] Key, byte[] Value)>
        {
            (KeyEncoder.Encode("Person", "Name"), BinaryCodec.Encode("Alice")),
            (KeyEncoder.Encode("Person", "Age"), BinaryCodec.Encode(25)),
            (KeyEncoder.Encode("Person", "Address", "City"), BinaryCodec.Encode("NYC")),
            (KeyEncoder.Encode("Person", "Address", "Zip"), BinaryCodec.Encode("10001")),
        };

        var result = GarrySerializer.Deserialize<Person>("Person", pairs);
        Assert.NotNull(result);
        Assert.NotNull(result!.Address);
        Assert.Equal("NYC", result.Address!.City);
        Assert.Equal("10001", result.Address.Zip);
    }

    [Fact]
    public void RoundTrip_SerializeThenDeserialize_PreservesData()
    {
        var original = new Person
        {
            Name = "Alice",
            Age = 25,
            Address = new Address { City = "NYC", Zip = "10001" }
        };

        var pairs = GarrySerializer.Serialize("Person", original);
        var restored = GarrySerializer.Deserialize<Person>("Person", pairs);

        Assert.NotNull(restored);
        Assert.Equal(original.Name, restored!.Name);
        Assert.Equal(original.Age, restored.Age);
        Assert.NotNull(restored.Address);
        Assert.Equal(original.Address!.City, restored.Address!.City);
        Assert.Equal(original.Address.Zip, restored.Address.Zip);
    }
}
