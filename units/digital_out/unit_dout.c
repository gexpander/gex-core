//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_dout.h"

#define DOUT_INTERNAL
#include "_dout_settings.h"
#include "_dout_init.h"

enum PinCmd_ {
    CMD_TEST = 0,
    CMD_SET = 1,
    CMD_CLEAR = 2,
    CMD_TOGGLE = 3,
};

/** Handle a request message */
static error_t DOut_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command,
                                  PayloadParser *pp)
{
    uint16_t packed = pp_u16(pp);

    switch (command) {
        case CMD_TEST:
            return UU_DOut_Write(unit, packed);

        case CMD_SET:
            return UU_DOut_Set(unit, packed);

        case CMD_CLEAR:
            return UU_DOut_Clear(unit, packed);

        case CMD_TOGGLE:
            return UU_DOut_Toggle(unit, packed);

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_DOUT = {
    .name = "DO",
    .description = "Digital output",
    // Settings
    .preInit = DOut_preInit,
    .cfgLoadBinary = DOut_loadBinary,
    .cfgWriteBinary = DOut_writeBinary,
    .cfgLoadIni = DOut_loadIni,
    .cfgWriteIni = DOut_writeIni,
    // Init
    .init = DOut_init,
    .deInit = DOut_deInit,
    // Function
    .handleRequest = DOut_handleRequest,
};
