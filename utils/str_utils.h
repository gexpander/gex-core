#ifndef PLATFORSTR_UTILS_H
#define PLATFORSTR_UTILS_H

#include <stdbool.h>
#include <string.h>
#include "stringbuilder.h"

static inline bool __attribute__((const))
streq(const char *restrict str1, const char *restrict str2)
{
    return strcmp(str1, str2) == 0;
}

static inline bool __attribute__((const))
strcaseq(const char *restrict str1, const char *restrict str2)
{
    return strcasecmp(str1, str2) == 0;
}

static inline bool __attribute__((const))
strneq(const char *restrict str1, const char *restrict str2, uint32_t limit)
{
    return strncmp(str1, str2, limit) == 0;
}

static inline bool __attribute__((const))
strncaseq(const char *restrict str1, const char *restrict str2, uint32_t limit)
{
    return strncasecmp(str1, str2, limit) == 0;
}

static inline bool __attribute__((const))
strstarts(const char *restrict str, const char *restrict prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

static inline char __attribute__((const))
last_char_n(const char *str, uint32_t nth)
{
    return str[strlen(str) - nth];
}

static inline char __attribute__((const))
last_char(const char *str)
{
    return str[strlen(str) - 1];
}

bool str_parse_yn(const char *str, bool *suc);
uint8_t str_parse_01(const char *str, const char *a, const char *b, bool *suc);
uint8_t str_parse_012(const char *str, const char *a, const char *b, const char *c, bool *suc);

#define str_01(cond, a, b) ((cond) ? (b) : (a))
#define str_yn(cond) ((cond) ? ("Y") : ("N"))

bool str_parse_pin(const char *str, char *targetName, uint8_t *targetNumber);

#endif
