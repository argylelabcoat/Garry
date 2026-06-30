using System;

namespace GarrySharp;

/// <summary>
/// Override the key subscript name for a property.
/// Without this attribute, the property name is used as-is.
/// </summary>
[AttributeUsage(AttributeTargets.Property)]
public sealed class GarryKeyAttribute : Attribute
{
    public string Name { get; }

    public GarryKeyAttribute(string name)
    {
        Name = name ?? throw new ArgumentNullException(nameof(name));
    }
}
