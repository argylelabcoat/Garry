using System;
using System.Collections.Generic;
using System.IO;

namespace MeowDB
{
    public sealed class MeowException : Exception
    {
        public int StatusCode { get; }

        public MeowException(int statusCode, string message)
            : base(message)
        {
            StatusCode = statusCode;
        }

        public MeowException(int statusCode, string message, Exception inner)
            : base(message, inner)
        {
            StatusCode = statusCode;
        }

        public static void ThrowIfError(int status)
        {
            if (status == 0) return;

            switch (status)
            {
                case 1:
                    throw new KeyNotFoundException("Key not found in MeowDB database.");
                case 2:
                    throw new TimeoutException("Transaction lock conflict in MeowDB database.");
                case 3:
                    throw new IOException("I/O error in MeowDB database.");
                case 4:
                    throw new InvalidDataException("Database corruption detected in MeowDB database.");
                case 5:
                    throw new ArgumentException("Invalid argument passed to MeowDB function.");
                case 6:
                    throw new OutOfMemoryException("Out of memory in MeowDB database.");
                default:
                    throw new MeowException(status, $"MeowDB error with status code {status}.");
            }
        }
    }
}
