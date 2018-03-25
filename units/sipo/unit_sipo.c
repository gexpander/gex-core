//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_sipo.h"

#define SIPO_INTERNAL

#include "_sipo_internal.h"

// ------------------------------------------------------------------------

enum SipoCmd_ {
    CMD_WRITE = 0,
    CMD_DIRECT_DATA = 1,
    CMD_DIRECT_SHIFT = 2,
    CMD_DIRECT_CLEAR = 3,
    CMD_DIRECT_STORE = 4,
};

/** Handle a request message */
static error_t USIPO_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    switch (command) {
        case CMD_WRITE:
            {
                uint32_t len;
                uint16_t terminal_packed = pp_u16(pp);
                const uint8_t *tail = pp_tail(pp, &len);
                TRY(UU_SIPO_Write(unit, (uint8_t *) tail, (uint16_t) len, terminal_packed));
            }
            return E_SUCCESS;

        case CMD_DIRECT_DATA:
            TRY(UU_SIPO_DirectData(unit, pp_u16(pp)));
            return E_SUCCESS;

        case CMD_DIRECT_CLEAR:
            TRY(UU_SIPO_DirectClear(unit));
            return E_SUCCESS;

        case CMD_DIRECT_SHIFT:
            TRY(UU_SIPO_DirectShift(unit));
            return E_SUCCESS;

        case CMD_DIRECT_STORE:
            TRY(UU_SIPO_DirectStore(unit));
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_SIPO = {
    .name = "SIPO",
    .description = "Shift register driver (595, 4094)",
    // Settings
    .preInit = USIPO_preInit,
    .cfgLoadBinary = USIPO_loadBinary,
    .cfgWriteBinary = USIPO_writeBinary,
    .cfgLoadIni = USIPO_loadIni,
    .cfgWriteIni = USIPO_writeIni,
    // Init
    .init = USIPO_init,
    .deInit = USIPO_deInit,
    // Function
    .handleRequest = USIPO_handleRequest,
};
