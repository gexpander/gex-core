//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_DIN_INTERNAL_H
#define GEX_F072_DIN_INTERNAL_H

#ifndef DIN_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Private data structure */
struct priv {
    char port_name;
    uint16_t pins; // pin mask
    uint16_t pulldown; // pull-downs (default is pull-up)
    uint16_t pullup; // pull-ups
    uint16_t trig_rise; // pins generating events on rising edge
    uint16_t trig_fall; // pins generating events on falling edge
    uint16_t trig_holdoff; // ms
    uint16_t def_auto; // initial auto triggers

    uint16_t arm_auto;   // pins armed for auto reporting
    uint16_t arm_single; // pins armed for single event
    uint16_t holdoff_countdowns[16]; // countdowns to arm for each pin in the bit map
    GPIO_TypeDef *port;
};

/** Allocate data structure and set defaults */
error_t DIn_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void DIn_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void DIn_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t DIn_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void DIn_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Finalize unit set-up */
error_t DIn_init(Unit *unit);

/** Tear down the unit */
void DIn_deInit(Unit *unit);

// ------------------------------------------------------------------------

/**
 * EXTI callback for pin change interrupts
 *
 * @param arg - the unit is passed here
 */
void DIn_handleExti(void *arg);

#endif //GEX_F072_DIN_INTERNAL_H
