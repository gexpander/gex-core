//
// Created by MightyPork on 2018/02/23.
//

#include "cfg_utils.h"
#include "hw_utils.h"
#include "utils/avrlibc.h"
#include "utils/str_utils.h"

/** Parse a string representation of a pin directly to a resource constant */
Resource cfg_pinrsc_parse(const char *str, bool *suc)
{
    char pname;
    uint8_t pnum;

    if (!cfg_portpin_parse(str, &pname, &pnum)) {
        *suc = false;
        return R_NONE;
    }

    return rsc_portpin2rsc(pname, pnum, suc);
}

/** Convert a resource to a pin name - uses a static buffer, result must not be stored! */
char *cfg_pinrsc_encode(Resource rsc)
{
    static char buf[4];
    uint32_t index = rsc - R_PA0;
    uint32_t portnum = (index/16);
    uint8_t pinnum = (uint8_t) (index % 16);
    if (portnum >= PORTS_COUNT) return "";
    buf[0] = (char) ('A' + portnum);
    if (pinnum>9) {
        buf[1] = '1';
        buf[2] = (char) ('0' + (pinnum - 10));
        buf[3] = 0;
    } else {
        buf[1] = (char) ('0' + pinnum);
        buf[2] = 0;
    }
    return buf;
}

/** Parse single pin */
bool cfg_portpin_parse(const char *value, char *targetName, uint8_t *targetNumber)
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

/** Parse port name */
bool cfg_port_parse(const char *value, char *targetName)
{
    *targetName = (uint8_t) value[0];
    if (!(*targetName >= 'A' && *targetName < 'A' + PORTS_COUNT)) return false;
    return true;
}

/** Parse a list of pin numbers with ranges and commans/semicolons to a bitmask */
uint32_t cfg_pinmask_parse_32(const char *value, bool *suc)
{
    uint32_t bits = 0;
    uint32_t acu = 0;
    bool inrange = false;
    uint32_t rangestart = 0;

    // shortcut if none are set
    if (value[0] == 0) return 0;

    char c;
    do {
        c = *value++;
        if (c == ' ' || c == '\t') {
            // skip
        }
        else if (c >= '0' && c <= '9') {
            acu = acu*10 + (c-'0');
        }
        else if (c == ',' || c == ';' || c == 0) {
            // end of number or range
            if (!inrange) rangestart = acu;

            // swap them if they're in the wrong order
            if (acu < rangestart) {
                uint32_t swp = acu;
                acu = rangestart;
                rangestart = swp;
            }

            if (rangestart > 31) rangestart = 31;
            if (acu > 31) acu = 31;

            for(uint32_t i=rangestart; i <= acu; i++) {
                bits |= 1<<i;
            }

            inrange = false;
            rangestart = 0;
            acu = 0;
        }
        else if (c == '-' || c == ':') {
            rangestart = acu;
            inrange = true;
            acu=0;
        } else {
            *suc = false;
        }
    } while (c != 0);

    return bits;
}

/** Convert a pin bitmask to the ASCII format understood by str_parse_pinmask() */
char *cfg_pinmask_encode(uint32_t pins, char *buffer, bool ascending)
{
    char *b = buffer;
    uint32_t start = 0;
    bool on = false;
    bool first = true;
    bool bit;

    // shortcut if none are set
    if (pins == 0) {
        buffer[0] = 0;
        return buffer;
    }

    if (ascending) {
        for (int32_t i = 0; i <= 32; i++) {
            if (i == 32) {
                bit = false;
            } else {
                bit = 0 != (pins & 1);
                pins >>= 1;
            }

            if (bit) {
                if (!on) {
                    start = (uint32_t) i;
                    on = true;
                }
            } else {
                if (on) {
                    if (!first) {
                        b += SPRINTF(b, ", ");
                    }

                    if (start == (uint32_t)(i - 1)) {
                        b += SPRINTF(b, "%"PRIu32, start);
                    } else {
                        b += SPRINTF(b, "%"PRIu32"-%"PRIu32, start, i - 1);
                    }

                    first = false;
                    on = false;
                }
            }
        }
    } else {
        for (int32_t i = 31; i >= -1; i--) {
            if (i == -1) {
                bit = false;
            } else {
                bit = 0 != (pins & 0x80000000);
                pins <<= 1;
            }

            if (bit) {
                if (!on) {
                    start = (uint32_t) i;
                    on = true;
                }
            } else {
                if (on) {
                    if (!first) {
                        b += SPRINTF(b, ", ");
                    }

                    if (start == (uint32_t) (i + 1)) {
                        b += SPRINTF(b, "%"PRIu32, start);
                    } else {
                        b += SPRINTF(b, "%"PRIu32"-%"PRIu32, start, i + 1);
                    }

                    first = false;
                    on = false;
                }
            }
        }
    }

    return buffer;
}

bool cfg_bool_parse(const char *str, bool *suc)
{
    // TODO implement strcasecmp without the locale crap from newlib and use it here
    if (streq(str, "Y")) return true;
    if (streq(str, "N")) return false;

    if (streq(str, "1")) return true;
    if (streq(str, "0")) return false;

    if (streq(str, "H")) return true;
    if (streq(str, "L")) return false;

    if (streq(str, "YES")) return true;
    if (streq(str, "NO")) return false;

    if (streq(str, "ON")) return true;
    if (streq(str, "OFF")) return false;

    *suc = false;
    return false;
}

/** Convert number to one of 2 options */
const char *cfg_enum2_encode(uint32_t n,
                             uint32_t na, const char *a,
                             uint32_t nb, const char *b)
{
    if (n == nb) return b;
    return a;
}

/** Convert number to one of 3 options */
const char *cfg_enum3_encode(uint32_t n,
                             uint32_t na, const char *a,
                             uint32_t nb, const char *b,
                             uint32_t nc, const char *c)
{
    if (n == nb) return b;
    if (n == nc) return c;
    return a;
}

/** Convert number to one of 4 options */
const char *cfg_enum4_encode(uint32_t n,
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

uint32_t cfg_enum2_parse(const char *value,
                         const char *a, uint32_t na,
                         const char *b, uint32_t nb,
                         bool *suc)
{
    if (streq(value, a)) return na;
    if (streq(value, b)) return nb;
    *suc = false;
    return na;
}

uint32_t cfg_enum3_parse(const char *value,
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

uint32_t cfg_enum4_parse(const char *value,
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

void cfg_hex_parse(uint8_t *dest, uint32_t count, const char *value, bool *suc)
{
    // discard possible leading 0x
    if (value[0] == '0' && value[1] == 'x') {
        value += 2;
    }

    uint8_t bytebuf = 0;
    for (uint32_t digit = 0; digit < count * 2;) {
        char v = *value;
        if (v != 0) value++;

        uint8_t nibble = 0;
        if (v == ' ' || v == '.' || v == ',' || v == '-' || v == ':') {
            continue; // junk
        }
        else if (v >= '0' && v <= '9') {
            nibble = (uint8_t) (v - '0');
        }
        else if (v >= 'a' && v <= 'f') {
            nibble = (uint8_t) (10 + (v - 'a'));
        }
        else if (v >= 'A' && v <= 'F') {
            nibble = (uint8_t) (10 + (v - 'A'));
        }
        else if (v == 0) {
            nibble = 0; // pad with zeros
        }
        else {
            *suc = false;
            return;
        }

        digit++;
        bytebuf <<= 4;
        bytebuf |= nibble;
        if ((digit % 2 == 0) && digit > 0) { // whole byte
            *dest++ = bytebuf;
            bytebuf = 0;
        }
    }
}
