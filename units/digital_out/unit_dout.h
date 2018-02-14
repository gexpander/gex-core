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

// --- high-speed variants, without type checking and spreading ---

/** Spread a packed word */
error_t UU_DOut_Spread(Unit *unit, uint16_t packed, uint16_t *spread_out);

// those are like the normal functions, but without driver checking and other verification (where applicable)
// also the argument must be already spread, which saves time if the same value is used repeatedly
uint16_t UU_DOut_Spread_HS(Unit *unit, uint16_t packed);
void UU_DOut_Write_HS(Unit *unit, uint16_t spread);
void UU_DOut_Set_HS(Unit *unit, uint16_t spread);
void UU_DOut_Clear_HS(Unit *unit, uint16_t spread);
void UU_DOut_Toggle_HS(Unit *unit, uint16_t spread);

#endif //U_DOUT_H
