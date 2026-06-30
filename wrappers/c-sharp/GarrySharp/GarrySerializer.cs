using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace GarrySharp;

/// <summary>
/// Flattens/reconstructs .NET objects to/from hierarchical key trees.
/// Container nodes (objects) have no value; leaf nodes (primitives) have binary values.
/// </summary>
public static class GarrySerializer
{
    /// <summary>
    /// Serialize an object to a list of (key, value) pairs.
    /// First pair is always the container marker for the root object.
    /// </summary>
    public static List<(byte[] Key, byte[] Value)> Serialize<T>(string rootKey, T obj) where T : class
    {
        if (obj is null) throw new ArgumentNullException(nameof(obj));

        var pairs = new List<(byte[] Key, byte[] Value)>();
        var rootParts = new[] { rootKey };

        // Root container marker (empty value signals HAS_CHILDREN)
        pairs.Add((KeyEncoder.Encode(rootParts), Array.Empty<byte>()));

        SerializeObject(rootParts, obj, pairs);
        return pairs;
    }

    private static void SerializeObject(string[] prefix, object obj, List<(byte[] Key, byte[] Value)> pairs)
    {
        var properties = obj.GetType().GetProperties(BindingFlags.Public | BindingFlags.Instance)
            .Where(p => p.CanRead && p.GetCustomAttribute<GarryIgnoreAttribute>() is null);

        foreach (var prop in properties)
        {
            var keyName = prop.GetCustomAttribute<GarryKeyAttribute>()?.Name ?? prop.Name;
            var keyParts = new string[prefix.Length + 1];
            prefix.CopyTo(keyParts, 0);
            keyParts[^1] = keyName;

            var value = prop.GetValue(obj);

            if (value is null)
            {
                continue;
            }
            else if (IsPrimitive(value.GetType()))
            {
                // Primitive leaf
                pairs.Add((KeyEncoder.Encode(keyParts), BinaryCodec.Encode(value)));
            }
            else if (value.GetType().IsArray)
            {
                // Array container
                pairs.Add((KeyEncoder.Encode(keyParts), Array.Empty<byte>()));
                SerializeArray(keyParts, value, pairs);
            }
            else
            {
                // Nested object container
                pairs.Add((KeyEncoder.Encode(keyParts), Array.Empty<byte>()));
                SerializeObject(keyParts, value, pairs);
            }
        }
    }

    private static void SerializeArray(string[] prefix, object arrayObj, List<(byte[] Key, byte[] Value)> pairs)
    {
        var array = (Array)arrayObj;
        for (int i = 0; i < array.Length; i++)
        {
            var element = array.GetValue(i)!;
            var indexParts = new string[prefix.Length + 1];
            prefix.CopyTo(indexParts, 0);
            indexParts[^1] = i.ToString();

            if (IsPrimitive(element.GetType()))
            {
                pairs.Add((KeyEncoder.Encode(indexParts), BinaryCodec.Encode(element)));
            }
            else
            {
                pairs.Add((KeyEncoder.Encode(indexParts), Array.Empty<byte>()));
                SerializeObject(indexParts, element, pairs);
            }
        }
    }

    /// <summary>
    /// Deserialize an object from pre-fetched key-value pairs.
    /// </summary>
    public static T? Deserialize<T>(string rootKey, IReadOnlyList<(byte[] Key, byte[] Value)> pairs) where T : class, new()
    {
        if (pairs is null || pairs.Count == 0)
            return new T();

        var result = new T();
        var props = typeof(T).GetProperties(BindingFlags.Public | BindingFlags.Instance)
            .Where(p => p.CanWrite && p.GetCustomAttribute<GarryIgnoreAttribute>() is null)
            .ToDictionary(p => p.GetCustomAttribute<GarryKeyAttribute>()?.Name ?? p.Name);

        // Phase 1: Collect array elements
        var arrayData = new Dictionary<string, SortedDictionary<int, (byte[] Value, string[] RemainingParts)>>();

        foreach (var (key, value) in pairs)
        {
            var parts = KeyEncoder.DecodeParts(key);
            if (parts.Length < 2 || value.Length == 0) continue;
            if (parts[0] != rootKey) continue;

            if (parts.Length >= 3 && props.TryGetValue(parts[1], out var arrayProp) && arrayProp.PropertyType.IsArray)
            {
                if (int.TryParse(parts[2], out int index))
                {
                    if (!arrayData.ContainsKey(parts[1]))
                        arrayData[parts[1]] = new SortedDictionary<int, (byte[] Value, string[] RemainingParts)>();
                    arrayData[parts[1]][index] = (value, parts.Skip(3).ToArray());
                    continue;
                }
            }

            // Phase 1b: Non-array processing
            if (parts.Length == 2 && props.TryGetValue(parts[1], out var leafProp))
            {
                leafProp.SetValue(result, BinaryCodec.Decode(value));
            }
            else if (parts.Length > 2 && props.TryGetValue(parts[1], out var nestedProp))
            {
                var nestedObj = nestedProp.GetValue(result) ?? Activator.CreateInstance(nestedProp.PropertyType);
                if (nestedObj is not null)
                {
                    SetNestedValue(nestedObj, parts.Skip(2).ToArray(), value);
                    nestedProp.SetValue(result, nestedObj);
                }
            }
        }

        // Phase 2: Create arrays with correct sizes
        foreach (var (propName, elements) in arrayData)
        {
            if (!props.TryGetValue(propName, out var prop) || !prop.PropertyType.IsArray) continue;

            var elementType = prop.PropertyType.GetElementType()!;
            var array = Array.CreateInstance(elementType, elements.Count);

            foreach (var kvp in elements)
            {
                if (IsPrimitive(elementType))
                {
                    array.SetValue(BinaryCodec.Decode(kvp.Value.Value), kvp.Key);
                }
                else
                {
                    var element = Activator.CreateInstance(elementType)!;
                    if (kvp.Value.RemainingParts.Length > 0)
                        SetNestedValue(element, kvp.Value.RemainingParts, kvp.Value.Value);
                    array.SetValue(element, kvp.Key);
                }
            }

            prop.SetValue(result, array);
        }

        return result;
    }

    private static void SetNestedValue(object obj, string[] remainingParts, byte[] value)
    {
        if (remainingParts.Length == 1)
        {
            // Leaf — set the property
            var prop = obj.GetType().GetProperty(remainingParts[0],
                BindingFlags.Public | BindingFlags.Instance);
            prop?.SetValue(obj, BinaryCodec.Decode(value));
        }
        else
        {
            // Recurse into nested object
            var prop = obj.GetType().GetProperty(remainingParts[0],
                BindingFlags.Public | BindingFlags.Instance);
            if (prop is not null)
            {
                var nested = prop.GetValue(obj) ?? Activator.CreateInstance(prop.PropertyType);
                if (nested is not null)
                {
                    SetNestedValue(nested, remainingParts.Skip(1).ToArray(), value);
                    prop.SetValue(obj, nested);
                }
            }
        }
    }

    private static bool IsPrimitive(Type type)
    {
        return type.IsPrimitive
            || type == typeof(string)
            || type == typeof(byte[])
            || type == typeof(DateTime)
            || type == typeof(Guid)
            || type == typeof(decimal);
    }
}
