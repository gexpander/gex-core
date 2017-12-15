//
// Created by MightyPork on 2017/12/04.
//

#ifndef GEX_STACKSMON_H
#define GEX_STACKSMON_H

#include "platform.h"

#if USE_STACK_MONITOR
void stackmon_check_canaries(void);
void stackmon_dump(void);
void stackmon_register(const char *description, void *buffer, uint32_t len);
#else
#define stackmon_check_canaries() do {} while(0)
#define stackmon_dump() do {} while(0)
#define stackmon_register(description, buffer, len) do {} while(0)
#endif

#endif //GEX_STACKSMON_H
