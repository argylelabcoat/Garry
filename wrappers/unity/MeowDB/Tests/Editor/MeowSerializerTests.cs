using System;
using System.Collections.Generic;
using NUnit.Framework;

namespace MeowDB.Tests
{
    [TestFixture]
    public class MeowSerializerTests
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
            public Address Address { get; set; }
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
            public string[] Hobbies { get; set; } = new string[0];
            public int[] Scores { get; set; } = new int[0];
        }

        [Test]
        public void Serialize_SimpleObject_ReturnsContainerPlusLeaves()
        {
            var person = new Person { Name = "Alice", Age = 25 };
            var pairs = MeowSerializer.Serialize("Person", person);

            Assert.That(pairs.Count, Is.EqualTo(3));

            var containerKey = KeyEncoder.DecodeParts(pairs[0].Key);
            Assert.That(containerKey[0], Is.EqualTo("Person"));

            var nameKey = KeyEncoder.DecodeParts(pairs[1].Key);
            Assert.That(nameKey, Is.EqualTo(new[] { "Person", "Name" }));

            var ageKey = KeyEncoder.DecodeParts(pairs[2].Key);
            Assert.That(ageKey, Is.EqualTo(new[] { "Person", "Age" }));
        }

        [Test]
        public void Serialize_NestedObject_CreatesSubContainers()
        {
            var person = new Person
            {
                Name = "Alice",
                Age = 25,
                Address = new Address { City = "NYC", Zip = "10001" }
            };
            var pairs = MeowSerializer.Serialize("Person", person);

            Assert.That(pairs.Count, Is.EqualTo(6));
        }

        [Test]
        public void Serialize_IgnoresMarkedProperties()
        {
            var person = new PersonWithIgnore { Name = "Alice", Secret = "hidden" };
            var pairs = MeowSerializer.Serialize("Person", person);

            Assert.That(pairs.Count, Is.EqualTo(2));
        }

        [Test]
        public void Serialize_UsesGarryKeyName()
        {
            var person = new PersonWithKeyAttr { Name = "Alice" };
            var pairs = MeowSerializer.Serialize("Person", person);

            var nameKey = KeyEncoder.DecodeParts(pairs[1].Key);
            Assert.That(nameKey[1], Is.EqualTo("person_name"));
        }

        [Test]
        public void Deserialize_EmptyPairs_ReturnsDefault()
        {
            var result = MeowSerializer.Deserialize<Person>(
                "Person",
                new List<KeyValuePair<byte[], byte[]>>());
            Assert.That(result, Is.Not.Null);
            Assert.That(result.Name, Is.EqualTo(""));
        }

        [Test]
        public void Deserialize_WithLeafPairs_ReconstructsObject()
        {
            var pairs = new List<KeyValuePair<byte[], byte[]>>
            {
                new KeyValuePair<byte[], byte[]>(KeyEncoder.Encode("Person", "Name"), BinaryCodec.Encode("Alice")),
                new KeyValuePair<byte[], byte[]>(KeyEncoder.Encode("Person", "Age"), BinaryCodec.Encode(25)),
            };

            var result = MeowSerializer.Deserialize<Person>("Person", pairs);
            Assert.That(result, Is.Not.Null);
            Assert.That(result.Name, Is.EqualTo("Alice"));
            Assert.That(result.Age, Is.EqualTo(25));
        }

        [Test]
        public void Deserialize_NestedObject_ReconstructsGraph()
        {
            var pairs = new List<KeyValuePair<byte[], byte[]>>
            {
                new KeyValuePair<byte[], byte[]>(KeyEncoder.Encode("Person", "Name"), BinaryCodec.Encode("Alice")),
                new KeyValuePair<byte[], byte[]>(KeyEncoder.Encode("Person", "Age"), BinaryCodec.Encode(25)),
                new KeyValuePair<byte[], byte[]>(KeyEncoder.Encode("Person", "Address", "City"), BinaryCodec.Encode("NYC")),
                new KeyValuePair<byte[], byte[]>(KeyEncoder.Encode("Person", "Address", "Zip"), BinaryCodec.Encode("10001")),
            };

            var result = MeowSerializer.Deserialize<Person>("Person", pairs);
            Assert.That(result, Is.Not.Null);
            Assert.That(result.Address, Is.Not.Null);
            Assert.That(result.Address.City, Is.EqualTo("NYC"));
            Assert.That(result.Address.Zip, Is.EqualTo("10001"));
        }

        [Test]
        public void RoundTrip_SerializeThenDeserialize_PreservesData()
        {
            var original = new Person
            {
                Name = "Alice",
                Age = 25,
                Address = new Address { City = "NYC", Zip = "10001" }
            };

            var pairs = MeowSerializer.Serialize("Person", original);
            var restored = MeowSerializer.Deserialize<Person>("Person", pairs);

            Assert.That(restored, Is.Not.Null);
            Assert.That(restored.Name, Is.EqualTo(original.Name));
            Assert.That(restored.Age, Is.EqualTo(original.Age));
            Assert.That(restored.Address, Is.Not.Null);
            Assert.That(restored.Address.City, Is.EqualTo(original.Address.City));
            Assert.That(restored.Address.Zip, Is.EqualTo(original.Address.Zip));
        }

        public class Level10 { public string DeepValue { get; set; } = ""; }
        public class Level9 { public Level10 Child { get; set; } = new Level10(); }
        public class Level8 { public Level9 Child { get; set; } = new Level9(); }
        public class Level7 { public Level8 Child { get; set; } = new Level8(); }
        public class Level6 { public Level7 Child { get; set; } = new Level7(); }
        public class Level5 { public Level6 Child { get; set; } = new Level6(); }
        public class Level4 { public Level5 Child { get; set; } = new Level5(); }
        public class Level3 { public Level4 Child { get; set; } = new Level4(); }
        public class Level2 { public Level3 Child { get; set; } = new Level3(); }
        public class Level1 { public Level2 Child { get; set; } = new Level2(); }

        [Test]
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

            var pairs = MeowSerializer.Serialize("Root", original);
            var restored = MeowSerializer.Deserialize<Level1>("Root", pairs);

            Assert.That(restored, Is.Not.Null);
            Assert.That(restored.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue, Is.EqualTo("found it"));
        }

        [Test]
        public void RoundTrip_ArrayProperties_PreservesAllElements()
        {
            var original = new PersonWithArrays
            {
                Name = "Bob",
                Hobbies = new[] { "reading", "gaming", "hiking" },
                Scores = new[] { 95, 87, 100, 72 }
            };

            var pairs = MeowSerializer.Serialize("Person", original);
            var restored = MeowSerializer.Deserialize<PersonWithArrays>("Person", pairs);

            Assert.That(restored, Is.Not.Null);
            Assert.That(restored.Name, Is.EqualTo("Bob"));
            Assert.That(restored.Hobbies.Length, Is.EqualTo(3));
            Assert.That(restored.Hobbies[0], Is.EqualTo("reading"));
            Assert.That(restored.Hobbies[1], Is.EqualTo("gaming"));
            Assert.That(restored.Hobbies[2], Is.EqualTo("hiking"));
            Assert.That(restored.Scores.Length, Is.EqualTo(4));
            Assert.That(restored.Scores, Is.EqualTo(new[] { 95, 87, 100, 72 }));
        }
    }
}
