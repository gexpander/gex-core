//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_touch.h"

#define TOUCH_INTERNAL
#include "_touch_internal.h"

// ------------------------------------------------------------------------

enum TouchCmd_ {
    //
};

/** Handle a request message */
static error_t UTOUCH_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    switch (command) {
        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/**
 * Handle update-tick (if configured in init)
 *
 * @param unit
 */
static void UTOUCH_updateTick(Unit *unit)
{
    //
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_TOUCH = {
    .name = "TOUCH",
    .description = "Capacitive touch sensing",
    // Settings
    .preInit = UTOUCH_preInit,
    .cfgLoadBinary = UTOUCH_loadBinary,
    .cfgWriteBinary = UTOUCH_writeBinary,
    .cfgLoadIni = UTOUCH_loadIni,
    .cfgWriteIni = UTOUCH_writeIni,
    // Init
    .init = UTOUCH_init,
    .deInit = UTOUCH_deInit,
    // Function
    .handleRequest = UTOUCH_handleRequest,
    .updateTick = UTOUCH_updateTick,
};
