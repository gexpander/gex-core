//
// Simple string testing / manipulation functions, mainly used when
// building/parsing the config INI files
//

#ifndef PLATFORSTR_UTILS_H
#define PLATFORSTR_UTILS_H

#include <stdbool.h>
#include <string.h>
#include "stringbuilder.h"

/**
 * Test equality two strings
 *
 * @param str1
 * @param str2
 * @param limit
 * @return
 */
static inline bool __attribute__((const))
streq(const char *restrict str1, const char *restrict str2)
{
    return strcmp(str1, str2) == 0;
}

/**
 * Test equality of two strings, case insensitive
 *
 * @param str1
 * @param str2
 * @param limit
 * @return
 */
static inline bool __attribute__((const))
strcaseq(const char *restrict str1, const char *restrict str2)
{
    return strcasecmp(str1, str2) == 0;
}

/**
 * Test if a string starts with another
 *
 * @param str
 * @param prefix
 * @param limit
 * @return
 */
static inline bool __attribute__((const))
strneq(const char *restrict str, const char *restrict prefix, uint32_t limit)
{
    return strncmp(str, prefix, limit) == 0;
}

/**
 * Test if a string starts with another, case insensitive
 *
 * @param str1
 * @param str2
 * @return match
 */
static inline bool __attribute__((const))
strncaseq(const char *restrict str1, const char *restrict str2, uint32_t limit)
{
    return strncasecmp(str1, str2, limit) == 0;
}

/**
 * Test if a string starts with another
 *
 * @param str
 * @param prefix
 * @return match
 */
static inline bool __attribute__((const))
strstarts(const char *restrict str, const char *restrict prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

/**
 * Get n-th character from the end
 *
 * @param str
 * @param nth - n-th character (1 means last char)
 * @return the char
 */
static inline char __attribute__((const))
last_char_n(const char *str, uint32_t nth)
{
    return str[strlen(str) - nth];
}

/**
 * Get last character of a string
 *
 * @param str
 * @return last char
 */
static inline char __attribute__((const))
last_char(const char *str)
{
    return str[strlen(str) - 1];
}

#endif
