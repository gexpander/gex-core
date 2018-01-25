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
 * @param unit
 * @param packed - output; the packed (right aligned) bits representing the pins, highest to lowest, are written here.
 * @return success
 */
error_t UU_DI_Read(Unit *unit, uint16_t *packed);

#endif //U_DIN_H
