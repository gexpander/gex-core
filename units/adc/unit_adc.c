//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_adc.h"

#define ADC_INTERNAL
#include "_adc_internal.h"

// ------------------------------------------------------------------------

enum TplCmd_ {
    CMD_READ_RAW = 0,
    CMD_READ_SMOOTHED = 1,

    CMD_GET_ENABLED_CHANNELS = 10,

    CMD_SETUP_TRIGGER = 20,
    CMD_ARM = 21,
    CMD_DISARM = 22,
    CMD_ABORT = 23, // abort any ongoing capture or stream
    CMD_FORCE_TRIGGER = 24,
    CMD_BLOCK_CAPTURE = 25,
    CMD_STREAM_START = 26,
    CMD_STREAM_STOP = 27,
    CMD_SET_SMOOTHING_FACTOR = 28,
};

/** Handle a request message */
static error_t UADC_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    struct priv *priv = unit->data;
    PayloadBuilder pb = pb_start(unit_tmp512, UNIT_TMP_LEN, NULL);

    switch (command) {
        /**
         * Get enabled channels.
         * Response: bytes with indices of enabled channels, ascending order.
         */
        case CMD_GET_ENABLED_CHANNELS:
            for (uint8_t i = 0; i < 18; i++) {
                if (priv->extended_channels_mask & (1 << i)) {
                    pb_u8(&pb, i);
                }
            }
            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        /**
         * Set smoothing factor 0-1000.
         * pld: u16:factor
         */
        case CMD_SET_SMOOTHING_FACTOR:
            {
                uint16_t fac = pp_u16(pp);
                if (fac > 1000) return E_BAD_VALUE;
                priv->avg_factor_as_float = fac / 1000.0f;
            }
            return E_SUCCESS;

        /**
         * Read raw values from the last measurement.
         * Response: interleaved (u8:channel, u16:value) for all channels
         */
        case CMD_READ_RAW:
            if(priv->opmode != ADC_OPMODE_IDLE && priv->opmode != ADC_OPMODE_ARMED) {
                return E_BUSY;
            }

            for (uint8_t i = 0; i < 18; i++) {
                if (priv->extended_channels_mask & (1 << i)) {
                    pb_u8(&pb, i);
                    pb_u16(&pb, priv->last_samples[i]);
                }
            }
            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        /**
         * Read smoothed values.
         * Response: interleaved (u8:channel, f32:value) for all channels
         */
        case CMD_READ_SMOOTHED:
            if(priv->opmode != ADC_OPMODE_IDLE && priv->opmode != ADC_OPMODE_ARMED) {
                return E_BUSY;
            }

            for (uint8_t i = 0; i < 18; i++) {
                if (priv->extended_channels_mask & (1 << i)) {
                    pb_u8(&pb, i);
                    pb_float(&pb, priv->averaging_bins[i]);
                }
            }
            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        /**
         * Configure a trigger. This is legal only if the current state is IDLE or ARMED (will re-arm).
         *
         * Payload:
         *   u8 - source channel
         *   u16 - triggering level
         *   u8 - edge to trigger on: 1-rising, 2-falling, 3-both
         *   u16 - pre-trigger samples count
         *   u32 - post-trigger samples count
         *   u16 - trigger hold-off in ms (dead time after firing, before it cna fire again if armed)
         *   u8(bool) - auto re-arm after firing and completing the capture
         */
        case CMD_SETUP_TRIGGER:
            dbg("> Setup trigger");
            if (priv->opmode != ADC_OPMODE_IDLE && priv->opmode != ADC_OPMODE_ARMED) return E_BUSY;

            {
                uint8_t source = pp_u8(pp);
                uint16_t level = pp_u16(pp);
                uint8_t edge = pp_u8(pp); // TODO test falling edge and dual edge
                uint16_t pretrig = pp_u16(pp); // TODO test pre-trigger ...
                uint32_t count = pp_u32(pp);
                uint16_t holdoff = pp_u16(pp); // TODO test hold-off ...
                bool auto_rearm = pp_bool(pp);

                if (source > 17) {
                    com_respond_str(MSG_ERROR, frame_id, "Invalid trig source");
                    return E_FAILURE;
                }

                if (0 == (priv->extended_channels_mask & (1 << source))) {
                    com_respond_str(MSG_ERROR, frame_id, "Channel not enabled");
                    return E_FAILURE;
                }

                if (level > 4095) {
                    com_respond_str(MSG_ERROR, frame_id, "Level out of range (0-4095)");
                    return E_FAILURE;
                }

                if (edge == 0 || edge > 3) {
                    com_respond_str(MSG_ERROR, frame_id, "Bad edge");
                    return E_FAILURE;
                }

                // XXX the max size may be too much
                uint16_t max_pretrig = (priv->dma_buffer_itemcount / priv->nb_channels);
                if (pretrig > max_pretrig) {
                    com_respond_snprintf(frame_id, MSG_ERROR,
                                         "Pretrig too large (max %d)", (int) max_pretrig);
                    return E_FAILURE;
                }

                priv->trigger_source = source;
                priv->trig_level = level;
                priv->trig_prev_level = priv->last_samples[source];
                priv->trig_edge = edge;
                priv->pretrig_len = pretrig;
                priv->trig_len = count;
                priv->trig_holdoff = holdoff;
                priv->auto_rearm = auto_rearm;
            }
            return E_SUCCESS;

        /**
         * Arm (permissible only if idle and the trigger is configured)
         */
        case CMD_ARM:
            dbg("> Arm");
            if(priv->opmode != ADC_OPMODE_IDLE) return E_BUSY;

            if (priv->trig_len == 0) {
                com_respond_str(MSG_ERROR, frame_id, "Trigger not configured.");
                return E_FAILURE;
            }

            // avoid firing immediately by the value jumping across the scale
            priv->trig_prev_level = priv->last_samples[priv->trigger_source];

            UADC_SwitchMode(unit, ADC_OPMODE_ARMED);
            return E_SUCCESS;

        /**
         * Dis-arm. Permissible only when idle or armed.
         * Switches to idle.
         */
        case CMD_DISARM:
            // TODO test
            dbg("> Disarm");
            if(priv->opmode == ADC_OPMODE_IDLE) {
                return E_SUCCESS; // already idle, success - no work to do
            }
            // capture in progress
            if(priv->opmode != ADC_OPMODE_ARMED) return E_BUSY;

            UADC_SwitchMode(unit, ADC_OPMODE_IDLE);
            return E_SUCCESS;

        /**
         * Abort any ongoing capture and dis-arm.
         */
        case CMD_ABORT:;
            // TODO test
            dbg("> Abort capture");
            enum uadc_opmode old_opmode = priv->opmode;
            UADC_SwitchMode(unit, ADC_OPMODE_IDLE);

            if (old_opmode == ADC_OPMODE_FIXCAPT ||
                old_opmode == ADC_OPMODE_STREAM ||
                old_opmode == ADC_OPMODE_TRIGD) {
                UADC_ReportEndOfStream(unit);
            }
            return E_SUCCESS;

        /**
         * Force a trigger (complete with pre-trigger capture and hold-off)
         * The reported edge will be 0b11, here meaning "manual trigger"
         */
        case CMD_FORCE_TRIGGER:
            // TODO test
            dbg("> Force trigger");
            if(priv->opmode == ADC_OPMODE_IDLE) {
                com_respond_str(MSG_ERROR, frame_id, "Not armed");
                return E_FAILURE;
            }
            if(priv->opmode != ADC_OPMODE_ARMED) return E_BUSY;

            UADC_HandleTrigger(unit, 0b11, PTIM_GetMicrotime());
            return E_SUCCESS;

        /**
         * Start a block capture (like manual trigger, but without pre-trigger and arming)
         *
         * Payload:
         *   u32 - sample count (for each channel)
         */
        case CMD_BLOCK_CAPTURE:
            // TODO test
            dbg("> Block cpt");
            if(priv->opmode != ADC_OPMODE_ARMED &&
                priv->opmode != ADC_OPMODE_IDLE) return E_BUSY;

            uint32_t count = pp_u32(pp);

            UADC_StartBlockCapture(unit, count, frame_id);
            return E_SUCCESS;

        /**
         * Start streaming (like block capture, but unlimited)
         * The stream can be terminated by the stop command.
         */
        case CMD_STREAM_START:
            // TODO test
            dbg("> Stream ON");
            if(priv->opmode != ADC_OPMODE_ARMED &&
               priv->opmode != ADC_OPMODE_IDLE) return E_BUSY;

            UADC_StartStream(unit, frame_id);
            return E_SUCCESS;

        /**
         * Stop a stream.
         */
        case CMD_STREAM_STOP:
            // TODO test
            dbg("> Stream OFF");
            if(priv->opmode != ADC_OPMODE_STREAM) {
                com_respond_str(MSG_ERROR, frame_id, "Not streaming");
                return E_FAILURE;
            }

            UADC_StopStream(unit);
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_ADC = {
    .name = "ADC",
    .description = "Analog/digital converter",
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
    .updateTick = UADC_updateTick,
};
