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
    CMD_GET_SAMPLE_RATE = 11,

    CMD_SETUP_TRIGGER = 20,
    CMD_ARM = 21,
    CMD_DISARM = 22,
    CMD_ABORT = 23, // abort any ongoing capture or stream
    CMD_FORCE_TRIGGER = 24,
    CMD_BLOCK_CAPTURE = 25,
    CMD_STREAM_START = 26,
    CMD_STREAM_STOP = 27,
    CMD_SET_SMOOTHING_FACTOR = 28,
    CMD_SET_SAMPLE_RATE = 29,
};

/** Handle a request message */
static error_t UADC_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    struct priv *priv = unit->data;
    PayloadBuilder pb = pb_start(unit_tmp512, UNIT_TMP_LEN, NULL);

    // TODO toggling individual channels - would require DMA re-init and various changes in the usage of the struct

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

        case CMD_SET_SAMPLE_RATE:
            {
                uint32_t freq = pp_u32(pp);
                if (freq == 0) return E_BAD_VALUE;

                TRY(UADC_SetSampleRate(unit, freq));
            }
            // Pass through - send back the obtained sample rate
        /**
         * Read the real used frequency, expressed as float.
         * May differ from the configured or requested value due to prescaller limitations.
         */
        case CMD_GET_SAMPLE_RATE:
            pb_u32(&pb, priv->real_frequency_int);
            pb_float(&pb, priv->real_frequency);
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

            if (priv->real_frequency_int > UADC_MAX_FREQ_FOR_AVERAGING) {
                com_respond_str(MSG_ERROR, frame_id, "Too fast for smoothing");
                return E_FAILURE;
            }

            for (uint8_t i = 0; i < 18; i++) {
                if (priv->extended_channels_mask & (1 << i)) {
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
            if (priv->opmode != ADC_OPMODE_IDLE &&
                priv->opmode != ADC_OPMODE_ARMED &&
                priv->opmode != ADC_OPMODE_REARM_PENDING) {
                return E_BUSY;
            }

            {
                const uint8_t source = pp_u8(pp);
                const uint16_t level = pp_u16(pp);
                const uint8_t edge = pp_u8(pp);
                const uint16_t pretrig = pp_u16(pp);
                const uint32_t count = pp_u32(pp);
                const uint16_t holdoff = pp_u16(pp);
                const bool auto_rearm = pp_bool(pp);

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
                const uint16_t max_pretrig = (priv->dma_buffer_itemcount / priv->nb_channels);
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
            uint8_t sticky = pp_u8(pp);

            if(priv->opmode == ADC_OPMODE_ARMED || priv->opmode == ADC_OPMODE_REARM_PENDING) {
                // We are armed or will re-arm promptly, act like the call succeeded
                // The auto flag is set regardless
            } else {
                if (priv->opmode != ADC_OPMODE_IDLE) {
                    return E_BUSY; // capture in progress
                }

                if (priv->trig_len == 0) {
                    com_respond_str(MSG_ERROR, frame_id, "Trigger not configured.");
                    return E_FAILURE;
                }

                UADC_SwitchMode(unit, ADC_OPMODE_ARMED);
            }

            if (sticky != 255) {
                priv->auto_rearm = (bool)sticky;
            }

            return E_SUCCESS;

        /**
         * Dis-arm. Permissible only when idle or armed.
         * Switches to idle.
         */
        case CMD_DISARM:
            dbg("> Disarm");

            priv->auto_rearm = false;

            if(priv->opmode == ADC_OPMODE_IDLE) {
                return E_SUCCESS; // already idle, success - no work to do
            }

            // capture in progress
            if (priv->opmode != ADC_OPMODE_ARMED &&
                priv->opmode != ADC_OPMODE_REARM_PENDING) {
                // Capture in progress, we already cleared auto rearm, so we're done for now
                // auto_rearm is checked in the EOS isr and if cleared, does not re-arm.
                return E_SUCCESS;
            }

            UADC_SwitchMode(unit, ADC_OPMODE_IDLE);
            return E_SUCCESS;

        /**
         * Abort any ongoing capture and dis-arm.
         */
        case CMD_ABORT:;
            dbg("> Abort capture");
            TRY(UU_ADC_AbortCapture(unit));
            return E_SUCCESS;

        /**
         * Force a trigger (complete with pre-trigger capture and hold-off)
         * The reported edge will be 0b11, here meaning "manual trigger"
         */
        case CMD_FORCE_TRIGGER:
            dbg("> Force trigger");
            // This is similar to block capture, but includes the pre-trig buffer and has fixed size based on trigger config
            // FORCE is useful for checking if the trigger is set up correctly
            if (priv->opmode != ADC_OPMODE_ARMED &&
                priv->opmode != ADC_OPMODE_IDLE &&
                priv->opmode != ADC_OPMODE_REARM_PENDING) return E_BUSY;

            if (priv->trig_len == 0) {
                com_respond_str(MSG_ERROR, frame_id, "Trigger not configured.");
                return E_FAILURE;
            }

            UADC_HandleTrigger(unit, 0b11, PTIM_GetMicrotime());
            return E_SUCCESS;

        /**
         * Start a block capture (like manual trigger, but without pre-trigger and arming)
         *
         * Payload:
         *   u32 - sample count (for each channel)
         */
        case CMD_BLOCK_CAPTURE:
            dbg("> Block cpt");
            if (priv->opmode != ADC_OPMODE_ARMED &&
                priv->opmode != ADC_OPMODE_REARM_PENDING &&
                priv->opmode != ADC_OPMODE_IDLE) return E_BUSY;

            uint32_t count = pp_u32(pp);

            UADC_StartBlockCapture(unit, count, frame_id);
            return E_SUCCESS;

        /**
         * Start streaming (like block capture, but unlimited)
         * The stream can be terminated by the stop command.
         */
        case CMD_STREAM_START:
            dbg("> Stream ON");
            if (priv->opmode != ADC_OPMODE_ARMED &&
                priv->opmode != ADC_OPMODE_REARM_PENDING &&
                priv->opmode != ADC_OPMODE_IDLE) return E_BUSY;

            UADC_StartStream(unit, frame_id);
            return E_SUCCESS;

        /**
         * Stop a stream.
         */
        case CMD_STREAM_STOP:
            dbg("> Stream OFF");
            if (priv->opmode != ADC_OPMODE_STREAM) {
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
