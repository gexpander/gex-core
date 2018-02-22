//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_fcap.h"

#define FCAP_INTERNAL
#include "_fcap_internal.h"

// ------------------------------------------------------------------------

enum FcapCmd_ {
    CMD_STOP = 0,

    // Measuring a waveform
    CMD_INDIRECT_CONT_START = 1,  // keep measuring, read on demand
    CMD_INDIRECT_BURST_START = 2, // wait and reply

    // Counting pulses
    CMD_DIRECT_CONT_START = 3,  // keep measuring, read on demand
    CMD_DIRECT_BURST_START = 4, // wait and reply
    CMD_FREECOUNT_START = 5,    // keep counting pulses until stopped, read on reply

    CMD_MEASURE_SINGLE_PULSE = 6, // measure the first incoming pulse of the right polarity. NOTE: can glitch if the signal starts in the active level
    CMD_FREECOUNT_CLEAR = 7, // clear the free counter, return last value

    // Results readout for continuous modes
    CMD_INDIRECT_CONT_READ = 10,
    CMD_DIRECT_CONT_READ = 11,
    CMD_FREECOUNT_READ = 12,

    // configs
    CMD_SET_POLARITY = 20,
    CMD_SET_DIR_PRESC = 21,
    CMD_SET_INPUT_FILTER = 22,
    CMD_SET_DIR_MSEC = 23,

    // go back to the configured settings
    CMD_RESTORE_DEFAULTS = 30,
};

/** Handle a request message */
static error_t UFCAP_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    uint8_t presc;
    uint16_t msec;

    struct priv *priv = unit->data;
    PayloadBuilder pb = pb_start(unit_tmp512, UNIT_TMP_LEN, NULL);
    const char* msg_denied_on_pin = "Not available on the selected pin!";

    switch (command) {
        /**
         * Stop any ongoing measurement and return to base state.
         */
        case CMD_STOP:
            UFCAP_SwitchMode(unit, OPMODE_IDLE);
            return E_SUCCESS;

        // ----------------------- CONFIG --------------------------

        /**
         * Set the active polarity, or triggering edge (for direct)
         *
         * pld: pol:u8 (0,1)
         */
        case CMD_SET_POLARITY:
            {
                priv->active_level = pp_bool(pp);
            }
            return E_SUCCESS;

        /**
         * Set the direct measurement prescaller 1,2,4,8
         *
         * pld: presc:u8
         */
        case CMD_SET_DIR_PRESC:
            {
                presc = pp_u8(pp);
                if (presc != 1 && presc != 2 && presc != 4 && presc != 8) return E_BAD_VALUE;
                priv->direct_presc = presc;
            }
            return E_SUCCESS;

        /**
         * Set the input filter for all modes
         *
         * pld: filter:u8 (0-15)
         */
        case CMD_SET_INPUT_FILTER:
            {
                uint8_t input_filter = pp_u8(pp);
                if (input_filter >= 16) return E_BAD_VALUE;
                priv->dfilter = input_filter;
            }
            return E_SUCCESS;

        /**
         * Set the direct sampling time.
         *
         * pld: msec:u16
         */
        case CMD_SET_DIR_MSEC:
            {
                msec = pp_u16(pp);
                priv->direct_msec = msec;
            }
            return E_SUCCESS;

        /**
         * Reset all SET* settings to their default values, stop any ongoing measure.
         */
        case CMD_RESTORE_DEFAULTS:
            UFCAP_SwitchMode(unit, OPMODE_IDLE);

            priv->active_level = priv->conf.active_level;
            priv->direct_presc = priv->conf.direct_presc;
            priv->direct_msec = priv->conf.direct_msec;
            priv->dfilter = priv->conf.dfilter;
            return E_SUCCESS;

        // ------------------ COMMANDS ------------------------

        /**
         * Start indirect continuous measurement.
         */
        case CMD_INDIRECT_CONT_START:
            if (priv->opmode == OPMODE_INDIRECT_CONT) return E_SUCCESS; // no-op
            if (priv->opmode != OPMODE_IDLE) return E_BUSY;

            UFCAP_SwitchMode(unit, OPMODE_INDIRECT_CONT);
            return E_SUCCESS;

        /**
         * Start a continuous direct measurement (counting pulses in fixed time intervals)
         *
         * - meas_time_ms 0 = no change
         * - prescaller 0 = no change
         *
         * pld: meas_time_ms:u16, prescaller:u8
         * - prescaller is 1,2,4,8; 0 = no change
         */
        case CMD_DIRECT_CONT_START:
            if (!priv->a_direct) {
                // This works only if we use the ETR pin. TIM2 shares CH1 with ETR.
                // If CH2 is selected as input, ETR is not available.
                com_respond_str(MSG_ERROR, frame_id, msg_denied_on_pin);
                return E_FAILURE;
            }
            if (priv->opmode == OPMODE_DIRECT_CONT) return E_SUCCESS; // no-op
            if (priv->opmode != OPMODE_IDLE) return E_BUSY;

            msec = pp_u16(pp);
            presc = pp_u8(pp);
            if (msec != 0) priv->direct_msec = msec;
            if (presc != 0) priv->direct_presc = presc;

            UFCAP_SwitchMode(unit, OPMODE_DIRECT_CONT);
            return E_SUCCESS;

        /**
         * Start a burst of direct measurements with averaging.
         * The measurement is performed on N consecutive pulses.
         *
         * pld: count:u16
         *
         * resp: core_mhz:u16, count:u16, period_sum:u64, ontime_sum:u64
         */
        case CMD_INDIRECT_BURST_START:
            if (priv->opmode != OPMODE_IDLE) return E_BAD_MODE;

            priv->ind_burst.n_target = pp_u16(pp);
            priv->request_id = frame_id;
            UFCAP_SwitchMode(unit, OPMODE_INDIRECT_BURST);
            return E_SUCCESS;

        /**
         * Start a single direct measurement of the given length (pulses in time period)
         * If 'prescaller' is not 0, it is changed via the param field.
         *
         * pld: meas_time_ms:u16, prescaller:u8
         * - prescaller is 1,2,4,8; 0 = no change
         *
         * resp: prescaller:u8, meas_time_ms:u16, pulse_count:u32
         */
        case CMD_DIRECT_BURST_START:
            if (priv->opmode != OPMODE_IDLE) return E_BAD_MODE;

            priv->dir_burst.msec = pp_u16(pp);

            presc = pp_u8(pp);
            if (presc != 0) priv->direct_presc = presc;

            priv->request_id = frame_id;
            UFCAP_SwitchMode(unit, OPMODE_DIRECT_BURST);
            return E_SUCCESS;

        /**
         * Measure a single pulse length of the given polarity.
         * Measures time from a rising to a falling edge (or falling to rising, if polarity is 0)
         *
         * resp: core_mhz:u16, ontime:u32
         */
        case CMD_MEASURE_SINGLE_PULSE:
            if (priv->opmode != OPMODE_IDLE) return E_BAD_MODE;
            priv->request_id = frame_id;
            UFCAP_SwitchMode(unit, OPMODE_SINGLE_PULSE);
            return E_SUCCESS;

        /**
         * Start a free-running pulse counter.
         *
         * pld: prescaller:u8
         * - prescaller is 1,2,4,8; 0 = no change
         */
        case CMD_FREECOUNT_START:
            if (priv->opmode != OPMODE_IDLE) return E_BAD_MODE;

            presc = pp_u8(pp);
            if (presc != 0) priv->direct_presc = presc;

            UFCAP_SwitchMode(unit, OPMODE_FREE_COUNTER);
            return E_SUCCESS;

        /**
         * Reset the free-running pulse counter.
         *
         * resp: last_val:u32
         */
        case CMD_FREECOUNT_CLEAR:
            if (priv->opmode != OPMODE_FREE_COUNTER) {
                return E_BAD_MODE;
            }

            pb_u32(&pb, UFCAP_FreeCounterClear(unit));
            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        // ------------------ READING ---------------------

        /**
         * Read the most recent pulse measurement during continuous indirect measure.
         *
         * resp: core_mhz:u16, period:u32, ontime:u32
         */
        case CMD_INDIRECT_CONT_READ:
            if (priv->opmode != OPMODE_INDIRECT_CONT) {
                return E_BAD_MODE;
            }
            if (priv->ind_cont.last_period == 0) {
                return E_BUSY;
            }

            pb_u16(&pb, PLAT_AHB_MHZ);
            pb_u32(&pb, priv->ind_cont.last_period);
            pb_u32(&pb, priv->ind_cont.last_ontime);

            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        /**
         * Read the most recent result of a continuous direct measurement.
         *
         * resp: prescaller:u8, meas_time_ms:u16, pulse_count:u32
         */
        case CMD_DIRECT_CONT_READ:
            if (!priv->a_direct) { // see above
                com_respond_str(MSG_ERROR, frame_id, msg_denied_on_pin);
                return E_FAILURE;
            }
            if (priv->opmode != OPMODE_DIRECT_CONT) return E_BAD_MODE;
            if (priv->dir_cont.last_count == 0) return E_BUSY;

            pb_u8(&pb, priv->direct_presc);
            pb_u16(&pb, priv->direct_msec);
            pb_u32(&pb, priv->dir_cont.last_count);

            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        /**
         * Read the current value of the free-running pulse counter.
         *
         * The timing may have a significant jitter, this function is practically useful only for
         * slow pulse sources (like a geiger counter, item counting etc)
         *
         * resp: count:u32
         */
        case CMD_FREECOUNT_READ:
            if (priv->opmode != OPMODE_FREE_COUNTER) {
                return E_BAD_MODE;
            }

            pb_u32(&pb, UFCAP_GetFreeCounterValue(unit));
            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Frequency capture */
const UnitDriver UNIT_FCAP = {
    .name = "FCAP",
    .description = "Frequency and pulse measurement",
    // Settings
    .preInit = UFCAP_preInit,
    .cfgLoadBinary = UFCAP_loadBinary,
    .cfgWriteBinary = UFCAP_writeBinary,
    .cfgLoadIni = UFCAP_loadIni,
    .cfgWriteIni = UFCAP_writeIni,
    // Init
    .init = UFCAP_init,
    .deInit = UFCAP_deInit,
    // Function
    .handleRequest = UFCAP_handleRequest,
};
