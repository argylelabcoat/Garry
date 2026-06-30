using System;
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

    public class PersonWithArrays
    {
        public string Name { get; set; } = "";
        public string[] Hobbies { get; set; } = Array.Empty<string>();
        public int[] Scores { get; set; } = Array.Empty<int>();
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

    // Deep nesting test classes (10+ levels)
    public class Level10 { public string DeepValue { get; set; } = ""; }
    public class Level9 { public Level10 Child { get; set; } = new(); }
    public class Level8 { public Level9 Child { get; set; } = new(); }
    public class Level7 { public Level8 Child { get; set; } = new(); }
    public class Level6 { public Level7 Child { get; set; } = new(); }
    public class Level5 { public Level6 Child { get; set; } = new(); }
    public class Level4 { public Level5 Child { get; set; } = new(); }
    public class Level3 { public Level4 Child { get; set; } = new(); }
    public class Level2 { public Level3 Child { get; set; } = new(); }
    public class Level1 { public Level2 Child { get; set; } = new(); }

    [Fact]
    public void RoundTrip_DeeplyNestedObject_PreservesAllLevels()
    {
        var original = new Level1
        {
            Child = new Level2
            {
                Child = new Level3
                {
                    Child = new Level4
                    {
                        Child = new Level5
                        {
                            Child = new Level6
                            {
                                Child = new Level7
                                {
                                    Child = new Level8
                                    {
                                        Child = new Level9
                                        {
                                            Child = new Level10 { DeepValue = "found it" }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        };

        var pairs = GarrySerializer.Serialize("Root", original);
        var restored = GarrySerializer.Deserialize<Level1>("Root", pairs);

        Assert.NotNull(restored);
        Assert.Equal("found it", restored!.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue);
    }

    [Fact]
    public void RoundTrip_ArrayProperties_PreservesAllElements()
    {
        var original = new PersonWithArrays
        {
            Name = "Bob",
            Hobbies = new[] { "reading", "gaming", "hiking" },
            Scores = new[] { 95, 87, 100, 72 }
        };

        var pairs = GarrySerializer.Serialize("Person", original);
        var restored = GarrySerializer.Deserialize<PersonWithArrays>("Person", pairs);

        Assert.NotNull(restored);
        Assert.Equal("Bob", restored!.Name);
        Assert.Equal(3, restored.Hobbies.Length);
        Assert.Equal("reading", restored.Hobbies[0]);
        Assert.Equal("gaming", restored.Hobbies[1]);
        Assert.Equal("hiking", restored.Hobbies[2]);
        Assert.Equal(4, restored.Scores.Length);
        Assert.Equal(new[] { 95, 87, 100, 72 }, restored.Scores);
    }
}
