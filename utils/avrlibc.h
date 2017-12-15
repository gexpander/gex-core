//
// Created by MightyPork on 2017/11/26.
//

#ifndef GEX_AVRLIBC_H_H
#define GEX_AVRLIBC_H_H

int avr_atoi(const char *p);
long avr_strtol(const char *nptr, char **endptr, register int base);
long avr_atol(const char *p);
double avr_strtod (const char * nptr, char ** endptr);
unsigned long avr_strtoul(const char *nptr, char **endptr, register int base);

#endif //GEX_AVRLIBC_H_H
