//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_touch.h"

#define TOUCH_INTERNAL
#include "_touch_internal.h"

// ------------------------------------------------------------------------

enum TouchCmd_ {
    CMD_READ=0,
    CMD_SET_BIN_THR=1,
    CMD_DISABLE_ALL_REPORTS=2,
    CMD_SET_DEBOUNCE_TIME=3,
    CMD_SET_HYSTERESIS=4,
    CMD_GET_CH_COUNT=10,
};

/** Handle a request message */
static error_t UTOUCH_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    struct priv* priv = unit->data;
    PayloadBuilder pb = pb_start(unit_tmp512, UNIT_TMP_LEN, NULL);

    switch (command) {
        /**
         * read the current touch pad values (smaller = higher capacity)
         *
         * resp: a list of u16 (order: group and pin, ascending)
         */
        case CMD_READ:
            if (priv->status == UTSC_STATUS_BUSY) return E_BUSY;
            if (priv->status == UTSC_STATUS_FAIL) return E_HW_TIMEOUT;

            for (int i = 0; i < 32; i++) {
                if (priv->all_channels_mask & (1<<i)) {
                    pb_u16(&pb, priv->readouts[i]);
                }
            }
            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        /**
         * Set thresholds for the button mode.
         *
         * pld: a list of u16 for the enabled channels (order: group and pin, ascending)
         */
        case CMD_SET_BIN_THR:
            for (int i = 0; i < 32; i++) {
                if (priv->all_channels_mask & (1<<i)) {
                    priv->bin_trig_cnt[i] = 0;
                    priv->binary_thr[i] = pp_u16(pp);
                    if (priv->readouts[i] >= (priv->binary_thr[i] + priv->binary_hysteresis)) {
                        priv->binary_active_bits |= 1<<i;
                    }
                }
            }
            return E_SUCCESS;

        /**
         * Set the debounce time in ms (replaces the default value from settings)
         *
         * pld: ms:u16
         */
        case CMD_SET_DEBOUNCE_TIME:
            priv->binary_debounce_ms = pp_u16(pp);
            return E_SUCCESS;

        /**
         * Set hysteresis (replaces the default value from settings)
         *
         * Hysteresis is added to the threshold value for the switch-off level
         * (switch-off happens when the measured value is exceeded - capacity of the pad drops)
         *
         * pld: hyst:u16
         */
        case CMD_SET_HYSTERESIS:
            priv->binary_hysteresis = pp_u16(pp);
            return E_SUCCESS;

        /**
         * Disable button mode reports. This effectively sets all thresholds to 0, disabling checking.
         */
        case CMD_DISABLE_ALL_REPORTS:
            for (int i = 0; i < 32; i++) {
                if (priv->all_channels_mask & (1<<i)) {
                    priv->binary_thr[i] = 0;
                    priv->bin_trig_cnt[i] = 0;
                }
            }
            priv->binary_active_bits = 0;
            return E_SUCCESS;

        /**
         * Get the number of configured touch pad channels
         *
         * resp: count:u8
         */
        case CMD_GET_CH_COUNT:;
            uint8_t nb = 0;
            for (int i = 0; i < 32; i++) {
                if (priv->all_channels_mask & (1<<i)) {
                    nb++;
                }
            }
            pb_u8(&pb, nb);
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
