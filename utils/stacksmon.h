//
// Created by MightyPork on 2017/12/04.
//
// Utility for monitoring usage levels of FreeRTOS stacks and printing it in a nice table
//

#ifndef GEX_STACKSMON_H
#define GEX_STACKSMON_H

#include "platform.h"

#if USE_STACK_MONITOR

/** Number of tracked stacks, max */
#define STACK_NUM 3

/**
 * Check canaries and trap if they're dead
 */
void stackmon_check_canaries(void);

/**
 * Dump stacks usage table
 */
void stackmon_dump(void);

/**
 * Register a stack to be monitored
 *
 * @param description - stack name
 * @param buffer - stack buffer
 * @param len - stack size in bytes
 */
void stackmon_register(const char *description, void *buffer, uint32_t len);

#else
#define stackmon_check_canaries() do {} while(0)
#define stackmon_dump() do {} while(0)
#define stackmon_register(description, buffer, len) do {} while(0)
#endif

#endif //GEX_STACKSMON_H
