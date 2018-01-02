//
// Created by MightyPork on 2017/12/15.
//

#ifndef GEX_INDICATORS_H
#define GEX_INDICATORS_H

#include "platform.h"

/**
 * Indicator (LED or blinking pattern)
 */
enum GEX_StatusIndicator {
    STATUS_NONE = 0,
    STATUS_FAULT,
    STATUS_DISK_BUSY,
    STATUS_DISK_ATTACHED,
    STATUS_DISK_REMOVED,
    STATUS_HEARTBEAT,
    _INDICATOR_COUNT
};

/**
 * Early init
 */
void Indicator_PreInit(void);

/**
 * Initialize the statis LED(s)
 */
void Indicator_Init(void);

/**
 * Set indicator ON
 *
 * @param indicator
 */
void Indicator_Effect(enum GEX_StatusIndicator indicator);

/**
 * Set indicator OFF
 *
 * @param indicator
 */
void Indicator_Off(enum GEX_StatusIndicator indicator);

/**
 * Ms tick for indicators
 */
void Indicator_Tick(void);

/**
 * Heartbeat callback from the main thread to indicate activity
 */
void Indicator_Heartbeat(void);

#endif //GEX_INDICATORS_H
