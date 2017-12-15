//
// Created by MightyPork on 2017/11/26.
//

#include "str_utils.h"
#include "avrlibc.h"

bool str_parse_yn(const char *str, bool *suc)
{
    if (streq(str, "Y")) return true;
    if (streq(str, "1")) return true;
    if (streq(str, "YES")) return true;

    if (streq(str, "N")) return false;
    if (streq(str, "0")) return false;
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

bool str_parse_pin(const char *value, char *targetName, uint8_t *targetNumber)
{
    // discard leading 'P'
    if (value[0] == 'P') {
        value++;
    }

    size_t len = strlen(value);
    if (len<2||len>3) return false;

    *targetName = (uint8_t) value[0];
    if (!(*targetName >= 'A' && *targetName <= 'H')) return false;

    // lets just hope it's OK
    *targetNumber = (uint8_t) avr_atoi(value + 1);
    return true;
}
