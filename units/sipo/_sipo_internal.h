//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_SIPO_INTERNAL_H
#define GEX_F072_SIPO_INTERNAL_H

#ifndef SIPO_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Private data structure */
struct priv {
    // settings
    char    store_pname;
    uint8_t store_pnum;
    bool    store_pol;   //!< Store pulse active edge

    char    shift_pname;
    uint8_t shift_pnum;
    bool    shift_pol;   //!< Shift clock active edge

    char    clear_pname;
    uint8_t clear_pnum;
    bool    clear_pol;   //!< Clear signal active level

    char     data_pname;
    uint16_t data_pins;

    // live fields
    uint32_t store_ll;
    uint32_t shift_ll;
    uint32_t clear_ll;

    GPIO_TypeDef *store_port;
    GPIO_TypeDef *shift_port;
    GPIO_TypeDef *clear_port;
    GPIO_TypeDef *data_port;

    uint8_t data_width;
};

/** Allocate data structure and set defaults */
error_t USIPO_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void USIPO_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void USIPO_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t USIPO_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void USIPO_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Finalize unit set-up */
error_t USIPO_init(Unit *unit);

/** Tear down the unit */
void USIPO_deInit(Unit *unit);

#endif //GEX_F072_SIPO_INTERNAL_H
