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

/**
 * Precision timer: get microtime as uint64_t
 * This timestamp should be monotonously increasing with a precision of ±0.5µs
 *
 * @return time in microseconds
 */
uint64_t PTIM_GetMicrotime(void);

#endif //GEX_F072_TIMEBASE_H
