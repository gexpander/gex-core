//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_tpl.h"

#define TPL_INTERNAL
#include "_tpl_internal.h"
#include "_tpl_settings.h"
#include "_tpl_init.h"

// ------------------------------------------------------------------------

enum TplCmd_ {
    //
};

/** Handle a request message */
static error_t TPL_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
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
static void TPL_updateTick(Unit *unit)
{
    //
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_TPL = {
    .name = "TPL",
    .description = "Template unit",
    // Settings
    .preInit = TPL_preInit,
    .cfgLoadBinary = TPL_loadBinary,
    .cfgWriteBinary = TPL_writeBinary,
    .cfgLoadIni = TPL_loadIni,
    .cfgWriteIni = TPL_writeIni,
    // Init
    .init = TPL_init,
    .deInit = TPL_deInit,
    // Function
    .handleRequest = TPL_handleRequest,
    .updateTick = TPL_updateTick,
};
