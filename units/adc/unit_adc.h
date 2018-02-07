//
// Created by MightyPork on 2017/11/25.
//
// Digital input unit; single or multiple pin read access on one port (A-F)
//

#ifndef U_TPL_H
#define U_TPL_H

#include "unit.h"

extern const UnitDriver UNIT_ADC;

error_t UU_ADC_AbortCapture(Unit *unit);

#endif //U_TPL_H
