//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_tpl.h"

#define TPL_INTERNAL
#include "_tpl_internal.h"

// ------------------------------------------------------------------------

enum TplCmd_ {
    //
};

/** Handle a request message */
static error_t UTPL_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command,
                                  PayloadParser *pp)
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
static void UTPL_updateTick(Unit *unit)
{
    //
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_TPL = {
    .name = "TPL",
    .description = "Template unit",
    // Settings
    .preInit = UTPL_preInit,
    .cfgLoadBinary = UTPL_loadBinary,
    .cfgWriteBinary = UTPL_writeBinary,
    .cfgLoadIni = UTPL_loadIni,
    .cfgWriteIni = UTPL_writeIni,
    // Init
    .init = UTPL_init,
    .deInit = UTPL_deInit,
    // Function
    .handleRequest = UTPL_handleRequest,
    .updateTick = UTPL_updateTick,
};
