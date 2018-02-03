//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_SPI_INIT_H
#define GEX_F072_SPI_INIT_H

#ifndef SPI_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Finalize unit set-up */
error_t SPI_init(Unit *unit);

/** Tear down the unit */
void SPI_deInit(Unit *unit);

#endif //GEX_F072_SPI_INIT_H
