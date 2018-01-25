//
// Created by MightyPork on 2017/11/26.
//
// Cortex-M utilities (low level stuff missing from CMSIS)
//

#ifndef GEX_CORTEX_UTILS_H
#define GEX_CORTEX_UTILS_H

#include "platform.h"

static inline bool inIRQ(void)
{
    return __get_IPSR() != 0;
}

/** Tight asm loop */
#define __asm_loop(cycles) \
do { \
    register uint32_t _count asm ("r4") = cycles; \
    asm volatile( \
        ".syntax unified\n" \
        ".thumb\n" \
        "0:" \
            "subs %0, #1\n" \
            "bne 0b\n" \
        : "+r" (_count)); \
} while(0)

#endif //GEX_CORTEX_UTILS_H
