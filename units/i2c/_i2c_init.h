//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_I2C_INIT_H
#define GEX_F072_I2C_INIT_H

#ifndef I2C_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Allocate data structure and set defaults */
error_t UI2C_preInit(Unit *unit);

/** Finalize unit set-up */
error_t UI2C_init(Unit *unit);

/** Tear down the unit */
void UI2C_deInit(Unit *unit);


#endif //GEX_F072_I2C_INIT_H
