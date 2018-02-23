//
// Created by MightyPork on 2018/02/23.
//

#ifndef GEX_F072_CFG_UTILS_H
#define GEX_F072_CFG_UTILS_H

#include "platform.h"
#include "rsc_enum.h"
#include "utils/avrlibc.h"

/**
 * Parse a pin name (e.g. PA0 or A0) to port name and pin number
 *
 * @param str - source string
 * @param targetName - output: port name (one character)
 * @param targetNumber - output: pin number 0-15
 * @return success
 */
bool cfg_portpin_parse(const char *str, char *targetName, uint8_t *targetNumber);

/**
 * Parse a string representation of a pin directly to a resource constant
 *
 * @param[in]  str - source string - e.g. PA0 or A0
 * @param[out] suc - written to false on failure
 * @return the parsed resource
 */
Resource cfg_pinrsc_parse(const char *str, bool *suc);

/**
 * Convert a resource to a pin name - uses a static buffer, result must not be stored!
 *
 * @param[in] rsc - resource to print
 * @return a pointer to a static buffer used for exporting the names
 */
char *cfg_pinrsc_encode(Resource rsc);

/**
 * Parse a port name (one character) - validates that it's within range
 *
 * @param value - source string
 * @param targetName - output: port name (one character)
 * @return success
 */
bool cfg_port_parse(const char *value, char *targetName);

/**
 * Parse a list of pin numbers with ranges and commands/semicolons to a bitmask.
 * Supported syntax:
 * - comma separated numbers
 * - numbers connected by dash or colon form a range (can be in any order)
 *
 * @param value - source string
 * @param suc - set to False if parsing failed
 * @return the resulting bitmap
 */
uint32_t cfg_pinmask_parse_32(const char *value, bool *suc);

/** same as cfg_pinmask_parse_32(), but with a cast to u16 */
static inline uint16_t cfg_pinmask_parse(const char *value, bool *suc)
{
    return (uint16_t) cfg_pinmask_parse_32(value, suc);
}

/**
 * Convert a pin bitmap to the ASCII format understood by str_parse_pinmask()
 *
 * @param[in]  pins - sparse pin map
 * @param[out] buffer - output string buffer
 * @param[in]  ascending - use ordering 0..31 rather than 31..0
 * @return the output buffer
 */
char *cfg_pinmask_encode(uint32_t pins, char *buffer, bool ascending);


/** Parse Y/N to bool */
bool cfg_bool_parse(const char *str, bool *suc);

/** Convert number to one of 4 options */
const char *cfg_enum2_encode(uint32_t n,
                             uint32_t na, const char *a,
                             uint32_t nb, const char *b);

/** Convert number to one of 4 options */
const char *cfg_enum3_encode(uint32_t n,
                             uint32_t na, const char *a,
                             uint32_t nb, const char *b,
                             uint32_t nc, const char *c);

/** Convert number to one of 4 options */
const char *cfg_enum4_encode(uint32_t n,
                             uint32_t na, const char *a,
                             uint32_t nb, const char *b,
                             uint32_t nc, const char *c,
                             uint32_t nd, const char *d);

/** Convert string to one of two numeric options */
uint32_t cfg_enum2_parse(const char *tpl,
                         const char *a, uint32_t na,
                         const char *b, uint32_t nb,
                         bool *suc);

/** Convert string to one of three numeric options */
uint32_t cfg_enum3_parse(const char *tpl,
                         const char *a, uint32_t na,
                         const char *b, uint32_t nb,
                         const char *c, uint32_t nc,
                         bool *suc);

/** Convert string to one of four numeric options */
uint32_t cfg_enum4_parse(const char *tpl,
                         const char *a, uint32_t na,
                         const char *b, uint32_t nb,
                         const char *c, uint32_t nc,
                         const char *d, uint32_t nd,
                         bool *suc);

/** Convert bool to a Y or N constant string */
#define str_yn(cond) ((cond) ? ("Y") : ("N"))


static inline uint8_t cfg_u8_parse(const char *value, bool *suc)
{
    return (uint8_t) avr_atoi(value);
}

static inline int8_t cfg_i8_parse(const char *value, bool *suc)
{
    return (int8_t) avr_atoi(value);
}

static inline uint16_t cfg_u16_parse(const char *value, bool *suc)
{
    return (uint16_t) avr_atoi(value);
}

static inline int16_t cfg_i16_parse(const char *value, bool *suc)
{
    return (int16_t) avr_atoi(value);
}

static inline uint32_t cfg_u32_parse(const char *value, bool *suc)
{
    return (uint32_t) avr_atoi(value);
}

static inline int32_t cfg_i32_parse(const char *value, bool *suc)
{
    return (int32_t) avr_atoi(value);
}

#endif //GEX_F072_CFG_UTILS_H
