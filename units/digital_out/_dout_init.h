//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_DOUT_INIT_H
#define GEX_F072_DOUT_INIT_H

#ifndef DOUT_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Finalize unit set-up */
error_t DOut_init(Unit *unit);

/** Tear down the unit */
void DOut_deInit(Unit *unit);

#endif //GEX_F072_DOUT_INIT_H
