//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_touch.h"

#define TOUCH_INTERNAL
#include "_touch_internal.h"

// ------------------------------------------------------------------------

enum TouchCmd_ {
    CMD_READ=0
};

/** Handle a request message */
static error_t UTOUCH_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    struct priv* priv = unit->data;

    switch (command) {
        case CMD_READ:
            if (priv->status == UTSC_STATUS_BUSY) return E_BUSY;
            if (priv->status == UTSC_STATUS_FAIL) return E_HW_TIMEOUT;

            PayloadBuilder pb = pb_start(unit_tmp512, UNIT_TMP_LEN, NULL);
            for (int i = 0; i < 32; i++) {
                if (priv->all_channels_mask & (1<<i)) {
                    pb_u16(&pb, priv->readouts[i]);
                }
            }
            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
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
