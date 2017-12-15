//
// Created by MightyPork on 2017/12/04.
//

#ifndef GEX_HEXDUMP_H
#define GEX_HEXDUMP_H

#include <stdint.h>

/**
 * HEXDUMP https://stackoverflow.com/a/7776146/2180189
 *
 * @param desc - description printed above the dump
 * @param addr - address to dump
 * @param len - number of bytes
 */
void hexDump(const char *restrict desc, const void *restrict addr, uint32_t len);

#endif //GEX_HEXDUMP_H
