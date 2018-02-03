//
// Created by MightyPork on 2018/01/29.
//

#ifndef GEX_F072_OW_INTERNAL_H
#define GEX_F072_OW_INTERNAL_H

#ifndef OW_INTERNAL
#error bad include!
#endif

#include "_ow_search.h"

/** Private data structure */
struct priv {
    char port_name;
    uint8_t pin_number;
    bool parasitic;

    GPIO_TypeDef *port;
    uint32_t ll_pin;

    TimerHandle_t busyWaitTimer; // timer used to wait for ds1820 measurement completion
    bool busy; // flag used when the timer is running
    uint32_t busyStart;
    TF_ID busyRequestId;
    struct ow_search_state searchState;
};

/** Load from a binary buffer stored in Flash */
void OW_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void OW_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t OW_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void OW_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
error_t OW_preInit(Unit *unit);

/** Finalize unit set-up */
error_t OW_init(Unit *unit);

/** Tear down the unit */
void OW_deInit(Unit *unit);

/** Callback for the FreeRTOS timer used to wait for device ready */
void OW_TimerCb(TimerHandle_t xTimer);

// ------------------------------------------------------------------------

#endif //GEX_F072_OW_INTERNAL_H
