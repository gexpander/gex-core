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
    STATUS_FAULT = 0,
    STATUS_USB_CONN,
    STATUS_USB_ACTIVITY,
    STATUS_DISK_BUSY,
    STATUS_DISK_ATTACHED,
    _INDICATOR_COUNT
};

/**
 * Early init
 */
void StatusLed_PreInit(void);

/**
 * Initialize the statis LED(s)
 */
void StatusLed_Init(void);

/**
 * Set indicator ON
 *
 * @param indicator
 */
void StatusLed_On(enum GEX_StatusIndicator indicator);

/**
 * Set indicator OFF
 *
 * @param indicator
 */
void StatusLed_Off(enum GEX_StatusIndicator indicator);

/**
 * Indicator set or reset
 *
 * @param indicator
 * @param set
 */
void StatusLed_Set(enum GEX_StatusIndicator indicator, bool set);

/**
 * Turn indicator ON for a given interval
 *
 * @param indicator
 * @param ms - time ON in ms
 */
void StatusLed_Flash(enum GEX_StatusIndicator indicator, uint32_t ms);

/**
 * Ms tick for indicators
 */
void StatusLed_Tick(void);

/**
 * Heartbeat callback from the main thread to indicate activity
 */
void StatusLed_Heartbeat(void);

#endif //GEX_INDICATORS_H
