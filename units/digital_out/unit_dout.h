//
// Created by MightyPork on 2017/11/25.
//

#ifndef U_DOUT_H
#define U_DOUT_H

#include "unit.h"

extern const UnitDriver UNIT_DOUT;

error_t UU_DO_Write(Unit *unit, uint16_t packed);

error_t UU_DO_Set(Unit *unit, uint16_t packed);

error_t UU_DO_Clear(Unit *unit, uint16_t packed);

error_t UU_DO_Toggle(Unit *unit, uint16_t packed);

error_t UU_DO_GetPinCount(Unit *unit, uint8_t *count);

#endif //U_DOUT_H
