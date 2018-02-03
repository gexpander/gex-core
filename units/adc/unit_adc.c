//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_adc.h"

#define ADC_INTERNAL
#include "_adc_internal.h"

// ------------------------------------------------------------------------

enum TplCmd_ {
    //
};

/** Handle a request message */
static error_t UADC_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    switch (command) {
        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_ADC = {
    .name = "ADC",
    .description = "Template unit",
    // Settings
    .preInit = UADC_preInit,
    .cfgLoadBinary = UADC_loadBinary,
    .cfgWriteBinary = UADC_writeBinary,
    .cfgLoadIni = UADC_loadIni,
    .cfgWriteIni = UADC_writeIni,
    // Init
    .init = UADC_init,
    .deInit = UADC_deInit,
    // Function
    .handleRequest = UADC_handleRequest,
};
