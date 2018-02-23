//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_NPX_INTERNAL_H
#define GEX_F072_NPX_INTERNAL_H

#ifndef NPX_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Private data structure */
struct priv {
    struct {
        Resource pin;
        uint16_t pixels;
    } cfg;

    uint32_t ll_pin;
    GPIO_TypeDef *port;
};

// ------------------------------------------------------------------------

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

// ------------------------------------------------------------------------

/** Finalize unit set-up */
error_t Npx_init(Unit *unit);

/** Tear down the unit */
void Npx_deInit(Unit *unit);

#endif //GEX_F072_NPX_INTERNAL_H
