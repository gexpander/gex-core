//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_I2C_SETTINGS_H
#define GEX_F072_I2C_SETTINGS_H

#ifndef I2C_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Load from a binary buffer stored in Flash */
void UI2C_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void UI2C_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UI2C_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void UI2C_writeIni(Unit *unit, IniWriter *iw);

#endif //GEX_F072_I2C_SETTINGS_H
