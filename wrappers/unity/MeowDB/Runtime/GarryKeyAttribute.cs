using System;

namespace MeowDB
{
    [AttributeUsage(AttributeTargets.Property)]
    public sealed class GarryKeyAttribute : Attribute
    {
        public string Name { get; }

        public GarryKeyAttribute(string name)
        {
            Name = name ?? throw new ArgumentNullException(nameof(name));
        }
    }
}
