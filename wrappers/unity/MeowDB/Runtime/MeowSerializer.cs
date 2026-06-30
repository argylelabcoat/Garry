using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace MeowDB
{
    public static class MeowSerializer
    {
        public static List<KeyValuePair<byte[], byte[]>> Serialize<T>(string rootKey, T obj) where T : class
        {
            if (obj == null) throw new ArgumentNullException(nameof(obj));

            var pairs = new List<KeyValuePair<byte[], byte[]>>();
            var rootParts = new[] { rootKey };

            pairs.Add(new KeyValuePair<byte[], byte[]>(KeyEncoder.Encode(rootParts), new byte[0]));

            SerializeObject(rootParts, obj, pairs);
            return pairs;
        }

        private static void SerializeObject(string[] prefix, object obj, List<KeyValuePair<byte[], byte[]>> pairs)
        {
            var properties = obj.GetType().GetProperties(BindingFlags.Public | BindingFlags.Instance)
                .Where(p => p.CanRead && p.GetCustomAttribute<GarryIgnoreAttribute>() == null);

            foreach (var prop in properties)
            {
                var keyAttr = prop.GetCustomAttribute<GarryKeyAttribute>();
                string keyName = keyAttr != null ? keyAttr.Name : prop.Name;
                var keyParts = new string[prefix.Length + 1];
                prefix.CopyTo(keyParts, 0);
                keyParts[keyParts.Length - 1] = keyName;

                object value = prop.GetValue(obj);

                if (value == null)
                {
                    continue;
                }
                else if (IsPrimitive(value.GetType()))
                {
                    pairs.Add(new KeyValuePair<byte[], byte[]>(KeyEncoder.Encode(keyParts), BinaryCodec.Encode(value)));
                }
                else if (value.GetType().IsArray)
                {
                    pairs.Add(new KeyValuePair<byte[], byte[]>(KeyEncoder.Encode(keyParts), new byte[0]));
                    SerializeArray(keyParts, value, pairs);
                }
                else
                {
                    pairs.Add(new KeyValuePair<byte[], byte[]>(KeyEncoder.Encode(keyParts), new byte[0]));
                    SerializeObject(keyParts, value, pairs);
                }
            }
        }

        private static void SerializeArray(string[] prefix, object arrayObj, List<KeyValuePair<byte[], byte[]>> pairs)
        {
            var array = (Array)arrayObj;
            for (int i = 0; i < array.Length; i++)
            {
                object element = array.GetValue(i);
                var indexParts = new string[prefix.Length + 1];
                prefix.CopyTo(indexParts, 0);
                indexParts[indexParts.Length - 1] = i.ToString();

                if (IsPrimitive(element.GetType()))
                {
                    pairs.Add(new KeyValuePair<byte[], byte[]>(KeyEncoder.Encode(indexParts), BinaryCodec.Encode(element)));
                }
                else
                {
                    pairs.Add(new KeyValuePair<byte[], byte[]>(KeyEncoder.Encode(indexParts), new byte[0]));
                    SerializeObject(indexParts, element, pairs);
                }
            }
        }

        public static T Deserialize<T>(string rootKey, IReadOnlyList<KeyValuePair<byte[], byte[]>> pairs) where T : class, new()
        {
            if (pairs == null || pairs.Count == 0)
                return new T();

            var result = new T();
            var props = typeof(T).GetProperties(BindingFlags.Public | BindingFlags.Instance)
                .Where(p => p.CanWrite && p.GetCustomAttribute<GarryIgnoreAttribute>() == null)
                .ToDictionary(p =>
                {
                    var attr = p.GetCustomAttribute<GarryKeyAttribute>();
                    return attr != null ? attr.Name : p.Name;
                });

            var arrayData = new Dictionary<string, SortedDictionary<int, KeyValuePair<byte[], string[]>>>();

            foreach (var kvp in pairs)
            {
                byte[] key = kvp.Key;
                byte[] value = kvp.Value;
                string[] parts = KeyEncoder.DecodeParts(key);
                if (parts.Length < 2 || value.Length == 0) continue;
                if (parts[0] != rootKey) continue;

                if (parts.Length >= 3 && props.ContainsKey(parts[1]))
                {
                    var prop = props[parts[1]];
                    if (prop.PropertyType.IsArray)
                    {
                        int index;
                        if (int.TryParse(parts[2], out index))
                        {
                            if (!arrayData.ContainsKey(parts[1]))
                                arrayData[parts[1]] = new SortedDictionary<int, KeyValuePair<byte[], string[]>>();
                            var remaining = new string[parts.Length - 3];
                            Array.Copy(parts, 3, remaining, 0, remaining.Length);
                            arrayData[parts[1]][index] = new KeyValuePair<byte[], string[]>((value, remaining));
                            continue;
                        }
                    }
                }

                if (parts.Length == 2 && props.ContainsKey(parts[1]))
                {
                    props[parts[1]].SetValue(result, BinaryCodec.Decode(value));
                }
                else if (parts.Length > 2 && props.ContainsKey(parts[1]))
                {
                    var nestedProp = props[parts[1]];
                    object nestedObj = nestedProp.GetValue(result) ?? Activator.CreateInstance(nestedProp.PropertyType);
                    if (nestedObj != null)
                    {
                        var remainingParts = new string[parts.Length - 2];
                        Array.Copy(parts, 2, remainingParts, 0, remainingParts.Length);
                        SetNestedValue(nestedObj, remainingParts, value);
                        nestedProp.SetValue(result, nestedObj);
                    }
                }
            }

            foreach (var kvp in arrayData)
            {
                string propName = kvp.Key;
                var elements = kvp.Value;
                if (!props.ContainsKey(propName)) continue;
                var prop = props[propName];
                if (!prop.PropertyType.IsArray) continue;

                Type elementType = prop.PropertyType.GetElementType();
                var array = Array.CreateInstance(elementType, elements.Count);

                foreach (var elem in elements)
                {
                    int idx = elem.Key;
                    byte[] val = elem.Value.Key;
                    string[] remParts = elem.Value.Value;

                    if (IsPrimitive(elementType))
                    {
                        array.SetValue(BinaryCodec.Decode(val), idx);
                    }
                    else
                    {
                        object element = Activator.CreateInstance(elementType);
                        if (remParts.Length > 0)
                            SetNestedValue(element, remParts, val);
                        array.SetValue(element, idx);
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
                var prop = obj.GetType().GetProperty(remainingParts[0],
                    BindingFlags.Public | BindingFlags.Instance);
                if (prop != null)
                    prop.SetValue(obj, BinaryCodec.Decode(value));
            }
            else
            {
                var prop = obj.GetType().GetProperty(remainingParts[0],
                    BindingFlags.Public | BindingFlags.Instance);
                if (prop != null)
                {
                    object nested = prop.GetValue(obj) ?? Activator.CreateInstance(prop.PropertyType);
                    if (nested != null)
                    {
                        var childParts = new string[remainingParts.Length - 1];
                        Array.Copy(remainingParts, 1, childParts, 0, childParts.Length);
                        SetNestedValue(nested, childParts, value);
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
}
