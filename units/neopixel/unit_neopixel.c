//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_neopixel.h"

#define NPX_INTERNAL
#include "_npx_settings.h"
#include "_npx_init.h"

enum PinCmd_ {
    CMD_CLEAR = 0,
    CMD_LOAD = 1,
    CMD_LOAD_U32_LE = 2,
    CMD_LOAD_U32_BE = 3,
    CMD_GET_LEN = 4,
};

/** Handle a request message */
static error_t Npx_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    uint32_t len;
    const uint8_t *bytes;

    switch (command) {
        /** Clear the entire strip */
        case CMD_CLEAR:
            return UU_Npx_Clear(unit);

        /** Load packed RGB colors (length must match the strip size) */
        case CMD_LOAD:;
            bytes = pp_tail(pp, &len);
            return UU_Npx_Load(unit, bytes, len);

        /** Load sparse (uint32_t) colors, 0x00RRGGBB, little endian. */
        case CMD_LOAD_U32_LE:
            bytes = pp_tail(pp, &len);
            return UU_Npx_LoadU32LE(unit, bytes, len);

        /** Load sparse (uint32_t) colors, 0x00RRGGBB, big endian. */
        case CMD_LOAD_U32_BE:
            bytes = pp_tail(pp, &len);
            return UU_Npx_LoadU32BE(unit, bytes, len);

        /** Get the Neopixel strip length */
        case CMD_GET_LEN:;
            uint16_t count;
            TRY(UU_Npx_GetCount(unit, &count));
            com_respond_u16(frame_id, count);
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_NEOPIXEL = {
    .name = "NPX",
    .description = "Neopixel RGB LED strip",
    // Settings
    .preInit = Npx_preInit,
    .cfgLoadBinary = Npx_loadBinary,
    .cfgWriteBinary = Npx_writeBinary,
    .cfgLoadIni = Npx_loadIni,
    .cfgWriteIni = Npx_writeIni,
    // Init
    .init = Npx_init,
    .deInit = Npx_deInit,
    // Function
    .handleRequest = Npx_handleRequest,
};
