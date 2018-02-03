//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_OW_SETTINGS_H
#define GEX_F072_OW_SETTINGS_H

#ifndef OW_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Allocate data structure and set defaults */
error_t OW_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void OW_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void OW_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t OW_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void OW_writeIni(Unit *unit, IniWriter *iw);

#endif //GEX_F072_OW_SETTINGS_H
