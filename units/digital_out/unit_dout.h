//
// Created by MightyPork on 2017/11/25.
//
// Digital output unit; single or multiple pin write access on one port (A-F)
//

#ifndef U_DOUT_H
#define U_DOUT_H

#include "unit.h"

extern const UnitDriver UNIT_DOUT;

/**
 * Write pins (e.g. writing 0b10 if configured pins are PA5 and PA0 sets PA5=1,PA0=0)
 *
 * @param unit
 * @param packed - packed pin states (aligned to right)
 * @return success
 */
error_t UU_DOut_Write(Unit *unit, uint16_t packed);

/**
 * Set pins (clear none)
 *
 * @param unit
 * @param packed - packed pins, 1 if pin should be set
 * @return success
 */
error_t UU_DOut_Set(Unit *unit, uint16_t packed);

/**
 * Clear multiple pins
 *
 * @param unit
 * @param packed - packed pins, 1 if pin should be cleared
 * @return
 */
error_t UU_DOut_Clear(Unit *unit, uint16_t packed);

/**
 * Toggle pins
 *
 * @param unit
 * @param packed - packed pins, 1 if pin should be toggled
 * @return
 */
error_t UU_DOut_Toggle(Unit *unit, uint16_t packed);

/**
 * Get number of configured pins
 *
 * @param unit
 * @param count output, written with 0-16
 * @return success
 */
error_t UU_DOut_GetPinCount(Unit *unit, uint8_t *count);

/**
 * Send a pulse
 *
 * @param unit
 * @param packed - pins to pulse
 * @param polarity - pulse active level
 * @param is_usec - use usec precision (for < 1 ms)
 * @param count - number of units (msec or usec)
 * @return success
 */
error_t UU_DOut_Pulse(Unit *unit, uint16_t packed, bool polarity, bool is_usec, uint16_t count);

#endif //U_DOUT_H
