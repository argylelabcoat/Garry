#ifndef GARRY_TEST_HELPERS_H
#define GARRY_TEST_HELPERS_H
#include <stdio.h>
#include "storage_types.h"

static int garry_test_failures = 0;

#define GARRY_CHECK(cond)                                          \
    do                                                             \
    {                                                              \
        if (!(cond))                                               \
        {                                                          \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            garry_test_failures++;                                 \
        }                                                          \
    } while (0)

#include <string.h>

/* Port of test_helpers.rcsp: BYTE_FROM_INT(n) -> char_type(n) */
#ifdef __GNUC__
static __attribute__((unused)) garry_byte garry_test_byte_from_int(int n)
#else
static garry_byte garry_test_byte_from_int(int n)
#endif
{
    return (garry_byte)(n & 0xFF);
}

/* Port of runtime ENCODE_KEY("str") -> byte_array filled with string bytes + zero padding.
 * Widespread in tests for creating key/value byte_arrays from string literals.
 * C89 adaptation: uses output parameter (C89 cannot return arrays by value). */
#ifdef __GNUC__
static __attribute__((unused)) void garry_test_encode_key(garry_byte_array out, const char *s)
#else
static void garry_test_encode_key(garry_byte_array out, const char *s)
#endif
{
    size_t len;
    size_t i;
    memset(out, 0, 256);
    len = strlen(s);
    if (len > 256)
        len = 256;
    for (i = 0; i < len; i++)
    {
        out[i] = (garry_byte)(unsigned char)s[i];
    }
}

#define ENCODE_KEY(out, s) garry_test_encode_key(out, s)
#define BYTE_FROM_INT(n)   garry_test_byte_from_int(n)

#endif /* GARRY_TEST_HELPERS_H */
