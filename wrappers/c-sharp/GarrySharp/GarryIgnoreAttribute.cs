using System;

namespace GarrySharp;

/// <summary>
/// Exclude a property from serialization.
/// </summary>
[AttributeUsage(AttributeTargets.Property)]
public sealed class GarryIgnoreAttribute : Attribute { }
