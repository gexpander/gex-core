//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_SPI_SETTINGS_H
#define GEX_F072_SPI_SETTINGS_H

#ifndef SPI_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Allocate data structure and set defaults */
error_t SPI_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void SPI_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void SPI_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t SPI_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void SPI_writeIni(Unit *unit, IniWriter *iw);

#endif //GEX_F072_SPI_SETTINGS_H
