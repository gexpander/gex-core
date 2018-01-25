//
// Created by MightyPork on 2017/11/26.
//
// Those are low memory footprint implementations of some stdlib functions
// taken from the AVR libc. They are used instead of newlib versions.
//

#ifndef GEX_AVRLIBC_H_H
#define GEX_AVRLIBC_H_H

/**
 * atoi() - parse decimal int from ASCII
 *
 * @param p - string
 * @return int, 0 on failure
 */
int avr_atoi(const char *p);

/**
 * atol() - parse decimal long int from ASCII
 *
 * @param p - string
 * @return int, 0 on failure
 */
long avr_atol(const char *p);

/**
 * strtol() - parse integer number form string.
 * this is internally called by atol and atoi
 *
 * 0x is allowed for bases 0 and 16
 *
 * @param nptr - string to parse
 * @param endptr - NULL or pointer to string where the end will be stored (first bad char)
 * @param base - base 2, 10, 16.... 0 for auto
 * @return the number
 */
long avr_strtol(const char *nptr, char **endptr, register int base);

/**
 * Parse double from ASCII
 *
 * @param nptr - string to parse
 * @param endptr - NULL or pointer to string where the end will be stored (first bad char)
 * @return  the number
 */
double avr_strtod (const char * nptr, char ** endptr);

/**
 * like strtol(), but unsigned (and hence higher max value)
 *
 * @param nptr - string to parse
 * @param endptr - NULL or pointer to string where the end will be stored (first bad char)
 * @param base - base 2, 10, 16.... 0 for auto
 * @return the number
 */
unsigned long avr_strtoul(const char *nptr, char **endptr, register int base);

#endif //GEX_AVRLIBC_H_H
