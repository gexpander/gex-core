//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_NPX_INIT_H
#define GEX_F072_NPX_INIT_H

#ifndef NPX_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Finalize unit set-up */
error_t Npx_init(Unit *unit);

/** Tear down the unit */
void Npx_deInit(Unit *unit);

#endif //GEX_F072_NPX_INIT_H
