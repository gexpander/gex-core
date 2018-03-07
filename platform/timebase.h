//
// Created by MightyPork on 2018/01/27.
//
// Configures and manages the high priority timer used for timeouts and precision delays.
//
// SysTick can't be used for this because, under FreeRTOS, the system tick interrupt
// has the lowest priority to not interfere with more important application processes
// and interrupts.
//

#ifndef GEX_F072_TIMEBASE_H
#define GEX_F072_TIMEBASE_H

#include "platform.h"

#define TIMEBASE_TIMER TIM17

/**
 * Precision timer: get microtime as uint64_t
 * This timestamp should be monotonously increasing with a precision of ±0.5µs
 *
 * @return time in microseconds
 */
uint64_t PTIM_GetMicrotime(void);

/**
 * Get time in milliseconds since start
 *
 * @return time in ms
 */
static inline uint32_t PTIM_GetTime(void)
{
    return HAL_GetTick();
}

/**
 * Microseconds busy delay
 *
 * @param usec - max 998
 */
void PTIM_MicroDelay(uint32_t usec);

/**
 * Microsecond busy delay with alignment (reduced length jitter but possible up to 1 us delay)
 *
 * @param usec
 */
void PTIM_MicroDelayAligned(uint32_t usec, uint32_t start);

/**
 * Wait until the next micro timer tick
 */
static inline uint32_t PTIM_MicroDelayAlign(void)
{
    const uint32_t c = TIMEBASE_TIMER->CNT;
    uint32_t res;
    while ((res = TIMEBASE_TIMER->CNT) == c);
    return res;
}

#endif //GEX_F072_TIMEBASE_H
