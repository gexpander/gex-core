//
// Created by MightyPork on 2017/11/26.
//

#ifndef GEX_CORTEX_UTILS_H
#define GEX_CORTEX_UTILS_H

#include "platform.h"

static inline bool inIRQ(void)
{
    return __get_IPSR() != 0;
}

register char *__SP asm("sp");

static inline bool isDynAlloc(const void *obj)
{
    // this is the 0x20000000-something address that should correspond to heap bottom
    extern char heapstart __asm("end");

    return ((uint32_t)obj >= (uint32_t)&heapstart)
           && ((uint32_t)obj < (uint32_t)__SP);
}

/** Tight asm loop */
#define __asm_loop(cycles) \
do { \
    register uint32_t _count = cycles; \
    asm volatile( \
        ".syntax unified\n" \
        ".thumb\n" \
            "ldr r4, =%0\n" \
        "0:" \
            "subs r4, #1\n" \
            "bne 0b\n" \
        : /* no outputs */ : "g" (cycles) : "r4"); \
} while(0)

#endif //GEX_CORTEX_UTILS_H
