//
// Created by MightyPork on 2017/11/26.
//

#include "str_utils.h"
#include "platform.h"
#include "avrlibc.h"

bool str_parse_yn(const char *str, bool *suc)
{
    // TODO implement strcasecmp without the locale crap from newlib and use it here
    if (streq(str, "Y")) return true;
    if (streq(str, "N")) return false;

    if (streq(str, "1")) return true;
    if (streq(str, "0")) return false;

    if (streq(str, "YES")) return true;
    if (streq(str, "NO")) return false;

    *suc = false;
    return false;
}

uint8_t str_parse_01(const char *str, const char *a, const char *b, bool *suc)
{
    if (streq(str, a)) return 0;
    if (streq(str, b)) return 1;

    *suc = false;
    return 0;
}

uint8_t str_parse_012(const char *str, const char *a, const char *b, const char *c, bool *suc)
{
    if (streq(str, a)) return 0;
    if (streq(str, b)) return 1;
    if (streq(str, c)) return 2;

    *suc = false;
    return 0;
}

/** Convert number to one of 2 options */
const char *str_2(uint32_t n,
                  uint32_t na, const char *a,
                  uint32_t nb, const char *b)
{
    if (n == nb) return b;
    return a;
}

/** Convert number to one of 3 options */
const char *str_3(uint32_t n,
                  uint32_t na, const char *a,
                  uint32_t nb, const char *b,
                  uint32_t nc, const char *c)
{
    if (n == nb) return b;
    if (n == nc) return c;
    return a;
}

/** Convert number to one of 4 options */
const char *str_4(uint32_t n,
                  uint32_t na, const char *a,
                  uint32_t nb, const char *b,
                  uint32_t nc, const char *c,
                  uint32_t nd, const char *d)
{
    if (n == nb) return b;
    if (n == nc) return c;
    if (n == nd) return d;
    return a;
}

uint32_t str_parse_2(const char *value,
                     const char *a, uint32_t na,
                     const char *b, uint32_t nb,
                     bool *suc)
{
    if (streq(value, a)) return na;
    if (streq(value, b)) return nb;
    *suc = false;
    return na;
}

uint32_t str_parse_3(const char *value,
                     const char *a, uint32_t na,
                     const char *b, uint32_t nb,
                     const char *c, uint32_t nc,
                     bool *suc)
{
    if (streq(value, a)) return na;
    if (streq(value, b)) return nb;
    if (streq(value, c)) return nc;
    *suc = false;
    return na;
}

uint32_t str_parse_4(const char *value,
                     const char *a, uint32_t na,
                     const char *b, uint32_t nb,
                     const char *c, uint32_t nc,
                     const char *d, uint32_t nd,
                     bool *suc)
{
    if (streq(value, a)) return na;
    if (streq(value, b)) return nb;
    if (streq(value, c)) return nc;
    if (streq(value, d)) return nd;
    *suc = false;
    return na;
}
