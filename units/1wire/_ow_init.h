//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_OW_INIT_H
#define GEX_F072_OW_INIT_H

#ifndef OW_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Allocate data structure and set defaults */
error_t OW_preInit(Unit *unit);

/** Finalize unit set-up */
error_t OW_init(Unit *unit);

/** Tear down the unit */
void OW_deInit(Unit *unit);

extern void OW_TimerCb(TimerHandle_t xTimer);

#endif //GEX_F072_OW_INIT_H
