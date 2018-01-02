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

static inline void __delay_cycles(uint32_t cycles)
{
    uint32_t l = cycles/3;
    asm volatile( "0:" "sub %[count], #1;" "bne 0b;" :[count]"+r"(l) );
}

#endif //GEX_CORTEX_UTILS_H
