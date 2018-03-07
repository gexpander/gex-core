//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_DOUT_INTERNAL_H
#define GEX_F072_DOUT_INTERNAL_H

#ifndef DOUT_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Private data structure */
struct priv {
    char port_name;
    uint16_t pins; // pin mask
    uint16_t initial; // initial pin states
    uint16_t open_drain; // open drain pins

    GPIO_TypeDef *port;
    uint16_t msec_pulse_cnt[16];
    uint16_t msec_pulse_scheduled_1;
    uint16_t msec_pulse_scheduled_0;
};

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

// ------------------------------------------------------------------------

/** Finalize unit set-up */
error_t DOut_init(Unit *unit);

/** Tear down the unit */
void DOut_deInit(Unit *unit);

void DOut_Tick(Unit *unit);

#endif //GEX_F072_DOUT_INTERNAL_H
