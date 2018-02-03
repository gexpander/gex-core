//
// Created by MightyPork on 2017/11/25.
//
// Digital input unit; single or multiple pin read access on one port (A-F)
//

#ifndef U_DIN_H
#define U_DIN_H

#include "unit.h"

extern const UnitDriver UNIT_DIN;

/**
 * Read pins
 *
 * @param unit - unit instance
 * @param packed - output; the packed (right aligned) bits representing the pins, highest to lowest, are written here.
 * @return success
 */
error_t UU_DIn_Read(Unit *unit, uint16_t *packed);

/**
 * Arm pins for trigger generation
 *
 * @param unit - unit instance
 * @param arm_single_packed - packed bit map of pins to arm for single trigger
 * @param arm_auto_packed - packed bit map of pins to arm for auto trigger (repeated)
 * @return success
 */
error_t UU_DIn_Arm(Unit *unit, uint16_t arm_single_packed, uint16_t arm_auto_packed);

/**
 * Dis-arm pins to not generate events
 *
 * @param unit - unit instance
 * @param disarm_packed - packed bit map of pins to dis-arm
 * @return success
 */
error_t UU_DIn_DisArm(Unit *unit, uint16_t disarm_packed);

#endif //U_DIN_H
