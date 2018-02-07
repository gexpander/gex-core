//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_TPL_INTERNAL_H
#define GEX_F072_TPL_INTERNAL_H

#ifndef TPL_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Private data structure */
struct priv {
    // settings

    // internal state
};

/** Allocate data structure and set defaults */
error_t UTPL_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void UTPL_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void UTPL_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UTPL_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void UTPL_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Finalize unit set-up */
error_t UTPL_init(Unit *unit);

/** Tear down the unit */
void UTPL_deInit(Unit *unit);

#endif //GEX_F072_TPL_INTERNAL_H
