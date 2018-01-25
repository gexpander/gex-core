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

/** Parse Y/N to bool */
bool str_parse_yn(const char *str, bool *suc);

/** Compare string with two options */
uint8_t str_parse_01(const char *str, const char *a, const char *b, bool *suc);

/** Compare string with three options */
uint8_t str_parse_012(const char *str, const char *a, const char *b, const char *c, bool *suc);

/** Convert number to one of 4 options */
const char *str_2(uint32_t n,
                  uint32_t na, const char *a,
                  uint32_t nb, const char *b);

/** Convert number to one of 4 options */
const char *str_3(uint32_t n,
                  uint32_t na, const char *a,
                  uint32_t nb, const char *b,
                  uint32_t nc, const char *c);

/** Convert number to one of 4 options */
const char *str_4(uint32_t n,
                     uint32_t na, const char *a,
                     uint32_t nb, const char *b,
                     uint32_t nc, const char *c,
                     uint32_t nd, const char *d);

/** Convert string to one of two numeric options */
uint32_t str_parse_2(const char *tpl,
                     const char *a, uint32_t na,
                     const char *b, uint32_t nb,
                     bool *suc);

/** Convert string to one of three numeric options */
uint32_t str_parse_3(const char *tpl,
                     const char *a, uint32_t na,
                     const char *b, uint32_t nb,
                     const char *c, uint32_t nc,
                     bool *suc);

/** Convert string to one of four numeric options */
uint32_t str_parse_4(const char *tpl,
                     const char *a, uint32_t na,
                     const char *b, uint32_t nb,
                     const char *c, uint32_t nc,
                     const char *d, uint32_t nd,
                     bool *suc);

/** Convert bool to a Y or N constant string */
#define str_yn(cond) ((cond) ? ("Y") : ("N"))

#endif
