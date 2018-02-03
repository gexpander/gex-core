//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_TPL_INIT_H
#define GEX_F072_TPL_INIT_H

#ifndef TPL_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Finalize unit set-up */
error_t TPL_init(Unit *unit);

/** Tear down the unit */
void TPL_deInit(Unit *unit);

#endif //GEX_F072_TPL_INIT_H
