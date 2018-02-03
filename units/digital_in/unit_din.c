//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_din.h"

#define DIN_INTERNAL
#include "_din_internal.h"
#include "_din_settings.h"
#include "_din_init.h"

// ------------------------------------------------------------------------

enum PinCmd_ {
    CMD_READ = 0,
    CMD_ARM_SINGLE = 1,
    CMD_ARM_AUTO = 2,
    CMD_DISARM = 3,
};

/** Handle a request message */
static error_t DI_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    uint16_t pins = 0;

    switch (command) {
        case CMD_READ:;
            TRY(UU_DI_Read(unit, &pins));

            PayloadBuilder pb = pb_start((uint8_t*)unit_tmp512, UNIT_TMP_LEN, NULL);
            pb_u16(&pb, pins); // packed input pins
            com_respond_buf(frame_id, MSG_SUCCESS, (uint8_t *) unit_tmp512, pb_length(&pb));
            return E_SUCCESS;

        case CMD_ARM_SINGLE:;
            pins = pp_u16(pp);
            if (!pp->ok) return E_MALFORMED_COMMAND;

            TRY(UU_DI_Arm(unit, pins, 0));
            return E_SUCCESS;

        case CMD_ARM_AUTO:;
            pins = pp_u16(pp);
            if (!pp->ok) return E_MALFORMED_COMMAND;

            TRY(UU_DI_Arm(unit, 0, pins));
            return E_SUCCESS;

        case CMD_DISARM:;
            pins = pp_u16(pp);
            if (!pp->ok) return E_MALFORMED_COMMAND;

            TRY(UU_DI_DisArm(unit, pins));
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

/**
 * Decrement all the hold-off timers on tick
 *
 * @param unit
 */
static void DI_updateTick(Unit *unit)
{
    struct priv *priv = unit->data;

    for (int i = 0; i < 16; i++) {
        if (priv->holdoff_countdowns[i] > 0) {
            priv->holdoff_countdowns[i]--;
        }
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_DIN = {
    .name = "DI",
    .description = "Digital input with triggers",
    // Settings
    .preInit = DI_preInit,
    .cfgLoadBinary = DI_loadBinary,
    .cfgWriteBinary = DI_writeBinary,
    .cfgLoadIni = DI_loadIni,
    .cfgWriteIni = DI_writeIni,
    // Init
    .init = DI_init,
    .deInit = DI_deInit,
    // Function
    .handleRequest = DI_handleRequest,
    .updateTick = DI_updateTick,
};
