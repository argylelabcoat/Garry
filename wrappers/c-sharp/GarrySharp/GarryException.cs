using System;
using System.Collections.Generic;
using System.IO;

namespace GarrySharp;

/// <summary>
/// Exception thrown when a Garry operation fails.
/// </summary>
public sealed class GarryException : Exception
{
    public int StatusCode { get; }

    public GarryException(int statusCode, string message)
        : base(message)
    {
        StatusCode = statusCode;
    }

    public GarryException(int statusCode, string message, Exception inner)
        : base(message, inner)
    {
        StatusCode = statusCode;
    }

    /// <summary>
    /// Map a Garry status code to the appropriate C# exception.
    /// </summary>
    public static void ThrowIfError(int status)
    {
        if (status == 0) return; // GARRY_OK

        throw status switch
        {
            1 => new KeyNotFoundException("Key not found in Garry database."),
            2 => new TimeoutException("Transaction lock conflict in Garry database."),
            3 => new IOException("I/O error in Garry database."),
            4 => new InvalidDataException("Database corruption detected in Garry database."),
            5 => new ArgumentException("Invalid argument passed to Garry function."),
            6 => new OutOfMemoryException("Out of memory in Garry database."),
            _ => new GarryException(status, $"Garry error with status code {status}.")
        };
    }
}
