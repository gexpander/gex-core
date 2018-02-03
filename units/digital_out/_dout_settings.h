//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_DOUT_SETTINGS_H
#define GEX_F072_DOUT_SETTINGS_H

#ifndef DOUT_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Allocate data structure and set defaults */
error_t DOut_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void DOut_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void DOut_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t DOut_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void DOut_writeIni(Unit *unit, IniWriter *iw);

#endif //GEX_F072_DOUT_SETTINGS_H
