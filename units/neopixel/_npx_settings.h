//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_NPX_SETTINGS_H
#define GEX_F072_NPX_SETTINGS_H

#ifndef NPX_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Allocate data structure and set defaults */
error_t Npx_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void Npx_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void Npx_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t Npx_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void Npx_writeIni(Unit *unit, IniWriter *iw);

#endif //GEX_F072_NPX_SETTINGS_H
