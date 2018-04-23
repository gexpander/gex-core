//
// Created by MightyPork on 2018/02/04.
//
// The core functionality of the ADC unit is defined here.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_adc.h"

#define ADC_INTERNAL
#include "_adc_internal.h"

#define DMA_POS(priv) ((priv)->buf_itemcount - (priv)->DMA_CHx->CNDTR)

/**
 * Async job to send a chunk of the DMA buffer to PC.
 * This can't be done directly because the interrupt couldn't wait for the TinyFrame mutex.
 *
 * unit  - unit
 * data1 - start index
 * data2 - number of samples to send
 * data3 - bit flags: 0x80 if this is the last sample and we should close
 *                    0x01 if this was the TC interrupt (otherwise it's HT)
 */
static void UADC_JobSendBlockChunk(Job *job)
{
    Unit *unit = job->unit;
    struct priv *priv = unit->data;

    const uint32_t start = job->data1;
    const uint32_t count = job->data2;
    const bool close = (bool) (job->data3 & 0x80);
    const bool tc = (bool) (job->data3 & 0x01);

    const TF_TYPE type = close ? EVT_CAPT_DONE : EVT_CAPT_MORE;

    TF_Msg msg = {
        .frame_id = priv->stream_frame_id,
        .len = (TF_LEN) (1 /*seq*/ + count * sizeof(uint16_t)),
        .type = type,
    };

    assert_param(true == TF_Respond_Multipart(comm, &msg));
    TF_Multipart_Payload(comm, &priv->stream_serial, 1);
    TF_Multipart_Payload(comm, (uint8_t *) (priv->dma_buffer + start), count * sizeof(uint16_t));
    TF_Multipart_Close(comm);

    // Clear the "busy" flags - those are checked in the DMA ISR to detect overrun
    if (tc) priv->tc_pending = false;
    else priv->ht_pending = false;

    priv->stream_serial++;
}

/**
 * Async job to send the trigger header.
 * The header includes info about the trigger + the pre-trigger buffer.
 *
 * data1 - index in the DMA buffer at which the captured data willl start
 * data2 - edge type - 1 rise, 2 fall, 3 forced
 * timestamp - event stamp
 * unit - unit
 */
static void UADC_JobSendTriggerCaptureHeader(Job *job)
{
    Unit *unit = job->unit;
    struct priv *priv = unit->data;

    EventReport er = {
        .unit = unit,
        .type = EVT_CAPT_START,
        .timestamp = job->timestamp,
        .length = (priv->pretrig_len + ((priv->pretrig_len > 0)?1:0)) * // see below why +1
                      priv->nb_channels *
                      sizeof(uint16_t) +
                  4 /*pretrig len*/ +
                  1 /*edge*/ +
                  1 /* seq */
    };

    uint32_t index_trigd = job->data1;
    uint8_t edge = (uint8_t) job->data2;

    EventReport_Start(&er);
    priv->stream_frame_id = er.sent_msg_id;
    {
        // preamble
        uint8_t buf[6];
        PayloadBuilder pb = pb_start(buf, 6, NULL);
        pb_u32(&pb, priv->pretrig_len);
        pb_u8(&pb, edge);
        pb_u8(&pb, priv->stream_serial++); // This is the serial counter for the first chunk
                                           // (containing the pre-trigger, or empty if no pretrig configured)
        EventReport_PB(&pb);

        if (priv->pretrig_len > 0) {
            // pretrig

            // +1 because we want pretrig 0 to exactly start with the triggering sample
            uint32_t pretrig_remain = (priv->pretrig_len + 1) * priv->nb_channels;

            assert_param(index_trigd <= priv->buf_itemcount);

            // this is one past the last entry of the triggering capture group
            if (pretrig_remain > index_trigd) {
                // used items in the wrap-around part of the buffer
                uint32_t items_from_end = pretrig_remain - index_trigd;
                assert_param(priv->buf_itemcount - items_from_end >= index_trigd);

                EventReport_Data((uint8_t *) &priv->dma_buffer[priv->buf_itemcount - items_from_end],
                                 items_from_end * sizeof(uint16_t));

                assert_param(items_from_end <= pretrig_remain);
                pretrig_remain -= items_from_end;
            }

            assert_param(pretrig_remain <= index_trigd);
            EventReport_Data((uint8_t *) &priv->dma_buffer[index_trigd - pretrig_remain],
                             pretrig_remain * sizeof(uint16_t));
        }
    }
    EventReport_End();
}

/**
 * Async job to notify about end of stream
 */
static void UADC_JobSendEndOfStreamMsg(Job *job)
{
    TF_Msg msg = {
        .type = EVT_CAPT_DONE,
        .frame_id = (TF_ID) job->data1
    };
    TF_Respond(comm, &msg);
}

/**
 * Schedule sending a event report to the PC that the current stream has ended.
 * The client library should handle this appropriately.
 */
void UADC_ReportEndOfStream(Unit *unit)
{
    struct priv *priv = unit->data;

    Job j = {
        .unit = unit,
        .data1 = priv->stream_frame_id, // copy the ID, it may be invalid by the time the cb gets executed
        .cb = UADC_JobSendEndOfStreamMsg
    };
    scheduleJob(&j);
}

/**
 * This is a helper function for the ADC DMA interrupt for handing the different interrupt types (half / full transfer).
 * It sends the part of the buffer that was just captured via an async job, or aborts on overrun.
 *
 * It's split off here to allow calling it for the different flags without repeating code.
 *
 * @param unit
 * @param tc - true if this is the TC interrupt, else HT
 */
static void handle_httc(Unit *unit, bool tc)
{
    struct priv *priv = unit->data;
    uint32_t start = priv->stream_startpos;
    uint32_t end;
    const bool ht = !tc;

    const bool m_trigd = priv->opmode == ADC_OPMODE_TRIGD;
    const bool m_stream = priv->opmode == ADC_OPMODE_STREAM;
    const bool m_fixcpt = priv->opmode == ADC_OPMODE_BLCAP;

    if (ht) {
        end = (priv->buf_itemcount >> 1); // div2
    }
    else {
        end = priv->buf_itemcount;
    }

    if (start != end) { // this sometimes happened after a trigger, may be unnecessary now
        if (end < start) {
            // this was a trap for a bug with missed TC irq, it's hopefully fixed now
            trap("end < start! %d < %d, tc %d", (int)end, (int)start, (int)tc);
        }

        uint32_t sgcount = (end - start) / priv->nb_channels;

        if (m_trigd || m_fixcpt) {
            sgcount = MIN(priv->trig_stream_remain, sgcount);
            priv->trig_stream_remain -= sgcount;
        }

        // Check for the closing condition
        const bool close = !m_stream && priv->trig_stream_remain == 0;

        if ((tc && priv->tc_pending) || (ht && priv->ht_pending)) {
            dbg("(!) ADC DMA not handled in time, abort capture");
            UADC_SwitchMode(unit, ADC_OPMODE_EMERGENCY_SHUTDOWN);
            return;
        }

        // Here we set the tc/ht pending flags for detecting overrun

        Job j = {
            .unit = unit,
            .data1 = start,
            .data2 = sgcount * priv->nb_channels,
            .data3 = (uint32_t) (close*0x80) | (tc*1), // bitfields to indicate what's happening
            .cb = UADC_JobSendBlockChunk
        };

        if (tc)
            priv->tc_pending = true;
        else
            priv->ht_pending = true;

        if (!scheduleJob(&j)) {
            // Abort if we can't queue - the stream would tear and we'd hog the system with error messages
            UADC_SwitchMode(unit, ADC_OPMODE_EMERGENCY_SHUTDOWN);
            return;
        }

        if (close) {
            // If auto-arm is enabled, we need to re-arm again.
            // However, EOS irq is disabled during the capture so the trigger edge detection would
            // work on stale data from before this trigger. We have to wait for the next full
            // conversion (EOS) before arming.
            UADC_SwitchMode(unit, (priv->auto_rearm && m_trigd) ? ADC_OPMODE_REARM_PENDING : ADC_OPMODE_IDLE);
        }
    }

    // Advance the starting position
    if (tc) {
        priv->stream_startpos = 0;
    }
    else {
        priv->stream_startpos = priv->buf_itemcount >> 1; // div2
    }
}

/**
 * IRQ handler for the DMA flags.
 *
 * We handle flags:
 * TC - transfer complete
 * HT - half transfer
 * TE - transfer error (this should never happen unless there's a bug)
 *
 * The buffer works in a circular mode, so we always handle the previous half
 * or what of it should be sent (if capture started somewhere inside).
 *
 * @param arg - the unit, passed via the irq dispatcher
 */
void UADC_DMA_Handler(void *arg)
{
    Unit *unit = arg;
    struct priv *priv = unit->data;

    // First thing, grab the flags. They may change during the function.
    // Working on the live register might cause race conditions.
    const uint32_t isrsnapshot = priv->DMAx->ISR;

    if (priv->opmode == ADC_OPMODE_UNINIT) {
        // the IRQ occured while switching mode, clear flags and do nothing else
        LL_DMA_ClearFlag_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_ClearFlag_TC(priv->DMAx, priv->dma_chnum);
        LL_DMA_ClearFlag_TE(priv->DMAx, priv->dma_chnum);
        return;
    }

    if (LL_DMA_IsActiveFlag_G(isrsnapshot, priv->dma_chnum)) {
        // we have some flags set - check which
        const bool tc = LL_DMA_IsActiveFlag_TC(isrsnapshot, priv->dma_chnum);
        const bool ht = LL_DMA_IsActiveFlag_HT(isrsnapshot, priv->dma_chnum);
        const bool te = LL_DMA_IsActiveFlag_TE(isrsnapshot, priv->dma_chnum);

        if (ht) LL_DMA_ClearFlag_HT(priv->DMAx, priv->dma_chnum);
        if (tc) LL_DMA_ClearFlag_TC(priv->DMAx, priv->dma_chnum);

        if (te) {
            // this shouldn't happen - error
            adc_dbg("ADC DMA TE!");
            LL_DMA_ClearFlag_TE(priv->DMAx, priv->dma_chnum);
            return;
        }

        // check what mode we're in
        const bool m_trigd = priv->opmode == ADC_OPMODE_TRIGD;
        const bool m_stream = priv->opmode == ADC_OPMODE_STREAM;
        const bool m_fixcpt = priv->opmode == ADC_OPMODE_BLCAP;
        if (m_trigd || m_stream || m_fixcpt) {
            const uint32_t half = (uint32_t) (priv->buf_itemcount >> 1); // div2
            if (ht && tc) {
                // dual event interrupt - may happen if we missed both and they were pending after
                // interrupts became enabled again (this can happen due to the EOS or other higher prio irq's)

                if (priv->stream_startpos > half) {
                    handle_httc(unit, true); // TC
                    handle_httc(unit, false); // HT
                } else {
                    handle_httc(unit, false); // HT
                    handle_httc(unit, true); // TC
                }
            } else {
                if (ht && priv->stream_startpos > half) {
                    // We missed the TC interrupt while e.g. setting up the stream / interrupt. catch up!
                    // This fixes a bug with "negative size" for report.
                    handle_httc(unit, true); // TC
                }

                handle_httc(unit, tc);
            }
        } else {
            // This shouldn't happen, the interrupt should be disabled in this opmode
            dbg("(!) not streaming, ADC DMA IT should be disabled");
        }
    }
}

/**
 * End of measurement group interrupt handler.
 * This interrupt records the measured values and checks for trigger.
 *
 * @param arg - unit, passed b y irq dispatcher
 */
void UADC_ADC_EOS_Handler(void *arg)
{
    Unit *unit = arg;
    struct priv *priv = unit->data;

    const bool can_average = priv->cfg.enable_averaging &&
                             priv->real_frequency_int < UADC_MAX_FREQ_FOR_AVERAGING;

    // Normally
    uint64_t timestamp = 0;
    if (priv->opmode == ADC_OPMODE_ARMED) {
        timestamp = PTIM_GetMicrotime();
    }

    LL_ADC_ClearFlag_EOS(priv->ADCx);

    if (priv->opmode == ADC_OPMODE_UNINIT) {
        goto exit;
    }

    uint32_t dmapos = DMA_POS(priv);

    // Wait for the DMA to complete copying the last sample
    // XXX
    // experiments revealed this was actually a bug somewhere else and DMA
    // is quick enough so we don't have to worry about this
#if 0
    uint32_t err = (dmapos % priv->nb_channels);
    if (err != 0) {
        GPIOC->BSRR = 0x02;
        hw_wait_while(((dmapos = DMA_POS(priv)) % priv->nb_channels) != 0, 10);
        GPIOC->BRR = 0x02;
    }
#endif

    // wrap dmapos to be past the last sample, even if outside the buffer
    // - so we can subtract nb_channels
    uint32_t sample_pos;
    if (dmapos == 0) {
        sample_pos = (uint32_t) (priv->buf_itemcount);
    } else {
        sample_pos = dmapos;
    }
    sample_pos -= priv->nb_channels;

    uint16_t val;

#if 1
    for (uint32_t j = 0; j < priv->nb_channels; j++) {
        const uint8_t i = priv->channel_nums[j];
        val = priv->dma_buffer[sample_pos+j];

        if (can_average) {
            priv->averaging_bins[i] =
                priv->averaging_bins[i] * (1.0f - priv->avg_factor_as_float) +
                ((float) val) * priv->avg_factor_as_float;
        }

        priv->last_samples[i] = val;
    }
#else
    for (uint8_t i = 0; i < 18; i++) {
        if (channels_mask & (1 << i)) {
            val = priv->dma_buffer[sample_pos+cnt];

            cnt++;

            if (can_average) {
                priv->averaging_bins[i] =
                    priv->averaging_bins[i] * (1.0f - priv->avg_factor_as_float) +
                    ((float) val) * priv->avg_factor_as_float;
            }

            priv->last_samples[i] = val;
        }
    }
#endif

    switch (priv->opmode) {
        // Triggering condition test
        case ADC_OPMODE_ARMED:
            val =  priv->last_samples[priv->trigger_source];

            if ((priv->trig_prev_level < priv->trig_level) &&
                val >= priv->trig_level &&
                (bool) (priv->trig_edge & 0b01)) {
                // Rising edge
                UADC_HandleTrigger(unit, 0b01, timestamp);
            }
            else if ((priv->trig_prev_level > priv->trig_level) &&
                     val <= priv->trig_level &&
                     (bool) (priv->trig_edge & 0b10)) {
                // Falling edge
                UADC_HandleTrigger(unit, 0b10, timestamp);
            }
            priv->trig_prev_level = val;
            break;

        // auto-rearm was waiting for the next sample
        case  ADC_OPMODE_REARM_PENDING:
            if (!priv->auto_rearm) {
                // It looks like the flag was cleared by DISARM before we got a new sample.
                // Let's just switch to IDLE
                UADC_SwitchMode(unit, ADC_OPMODE_IDLE);
            } else {
                // Re-arming for a new trigger
                UADC_SwitchMode(unit, ADC_OPMODE_ARMED);
            }

        default:
            break;
    }

exit:
    return;
}

/**
 * Handle a detected trigger - start capture if we're not in hold-off
 *
 * @param unit
 * @param edge_type - edge type, is included in the report
 * @param timestamp - event time
 */
void UADC_HandleTrigger(Unit *unit, uint8_t edge_type, uint64_t timestamp)
{
    struct priv *priv = unit->data;
    if (priv->opmode == ADC_OPMODE_UNINIT) return;

    if (priv->trig_holdoff != 0 && priv->trig_holdoff_remain > 0) {
        // Trig discarded due to holdoff
        return;
    }

    if (priv->trig_holdoff > 0) {
        priv->trig_holdoff_remain = priv->trig_holdoff;
        // Start the tick
        unit->tick_interval = 1;
        unit->_tick_cnt = 1;
    }

    priv->stream_startpos = DMA_POS(priv);
    priv->trig_stream_remain = priv->trig_len;
    priv->stream_serial = 0;

    // This func may be called from the EOS interrupt, so it's safer to send the header message asynchronously
    Job j = {
        .unit = unit,
        .timestamp = timestamp,
        .data1 = priv->stream_startpos,
        .data2 = edge_type,
        .cb = UADC_JobSendTriggerCaptureHeader
    };
    scheduleJob(&j);

    UADC_SwitchMode(unit, ADC_OPMODE_TRIGD);
}

/**
 * Abort ongoing capture.
 */
void UADC_AbortCapture(Unit *unit)
{
    struct priv *priv = unit->data;

    const enum uadc_opmode old_opmode = priv->opmode;

    priv->auto_rearm = false;

    if (old_opmode == ADC_OPMODE_BLCAP ||
        old_opmode == ADC_OPMODE_STREAM ||
        old_opmode == ADC_OPMODE_TRIGD) {
        UADC_ReportEndOfStream(unit);
    }

    UADC_SwitchMode(unit, ADC_OPMODE_IDLE);
}

/**
 * Start a manual block capture.
 *
 * @param unit
 * @param len - number of samples (groups)
 * @param frame_id - TF session to re-use for the report (client has a listener set up)
 */
void UADC_StartBlockCapture(Unit *unit, uint32_t len, TF_ID frame_id)
{
    struct priv *priv = unit->data;
    if (priv->opmode == ADC_OPMODE_UNINIT) return;

    priv->stream_frame_id = frame_id;
    priv->stream_startpos = DMA_POS(priv);
    priv->trig_stream_remain = len;
    priv->stream_serial = 0;
    UADC_SwitchMode(unit, ADC_OPMODE_BLCAP);
}

/**
 * Start a stream
 *
 * @param frame_id - TF session to re-use for the frames (client has a listener set up)
 */
void UADC_StartStream(Unit *unit, TF_ID frame_id)
{
    struct priv *priv = unit->data;
    if (priv->opmode == ADC_OPMODE_UNINIT) return;

    priv->stream_frame_id = frame_id;
    UADC_SwitchMode(unit, ADC_OPMODE_STREAM);
}

/**
 * End a stream by user request.
 */
void UADC_StopStream(Unit *unit)
{
    struct priv *priv = unit->data;
    if (priv->opmode == ADC_OPMODE_UNINIT) return;

    UADC_ReportEndOfStream(unit);
    UADC_SwitchMode(unit, ADC_OPMODE_IDLE);
}

/**
 * Handle unit update tick - expire the trigger hold-off.
 * We also check for the emergency shutdown condition and clear it.
 */
void UADC_updateTick(Unit *unit)
{
    struct priv *priv = unit->data;

    // Recover from shutdown after a delay
    if (priv->opmode == ADC_OPMODE_EMERGENCY_SHUTDOWN) {
        adc_dbg("ADC recovering from emergency shutdown");

        UADC_ReportEndOfStream(unit);
        LL_TIM_EnableCounter(priv->TIMx);
        UADC_SwitchMode(unit, ADC_OPMODE_IDLE);
        unit->tick_interval = 0;
        return;
    }

    if (priv->trig_holdoff_remain > 0) {
        priv->trig_holdoff_remain--;

        if (priv->trig_holdoff_remain == 0) {
            unit->tick_interval = 0;
            unit->_tick_cnt = 0;
        }
    }
}

/**
 * Switch the ADC operational mode.
 *
 * @param unit
 * @param new_mode - mode to set
 */
void UADC_SwitchMode(Unit *unit, enum uadc_opmode new_mode)
{
    struct priv *priv = unit->data;

    const enum uadc_opmode old_mode = priv->opmode;
    if (new_mode == old_mode) return; // nothing to do

    // if un-itied, can go only to IDLE
    assert_param((old_mode != ADC_OPMODE_UNINIT) || (new_mode == ADC_OPMODE_IDLE));

    priv->opmode = ADC_OPMODE_UNINIT;

    if (new_mode == ADC_OPMODE_UNINIT) {
        adc_dbg("ADC switch -> UNINIT");
        // Stop the DMA, timer and disable ADC - this is called before tearing down the unit
        LL_TIM_DisableCounter(priv->TIMx);
        LL_ADC_ClearFlag_EOS(priv->ADCx);
        LL_ADC_DisableIT_EOS(priv->ADCx);

        // Switch off the ADC
        if (LL_ADC_IsEnabled(priv->ADCx)) {
            // Cancel ongoing conversion
            if (LL_ADC_REG_IsConversionOngoing(priv->ADCx)) {
                LL_ADC_REG_StopConversion(priv->ADCx);
                hw_wait_while(LL_ADC_REG_IsStopConversionOngoing(priv->ADCx), 100);
            }

            LL_ADC_Disable(priv->ADCx);
            hw_wait_while(LL_ADC_IsDisableOngoing(priv->ADCx), 100);
        }

        LL_DMA_DisableChannel(priv->DMAx, priv->dma_chnum);
        LL_DMA_DisableIT_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_DisableIT_TC(priv->DMAx, priv->dma_chnum);
        LL_DMA_ClearFlag_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_ClearFlag_TC(priv->DMAx, priv->dma_chnum);
    }
    else if (new_mode == ADC_OPMODE_IDLE || new_mode == ADC_OPMODE_REARM_PENDING) {
        // IDLE and ARMED are identical with the exception that the trigger condition is not checked
        // ARMED can be only entered from IDLE, thus we do the init only here.

        priv->tc_pending = false;
        priv->ht_pending = false;

        // In IDLE, we don't need the DMA interrupts
        LL_DMA_ClearFlag_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_ClearFlag_TC(priv->DMAx, priv->dma_chnum);
        LL_DMA_DisableIT_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_DisableIT_TC(priv->DMAx, priv->dma_chnum);

        // Use End Of Sequence to recover results for averaging from the DMA buffer and DR
        LL_ADC_ClearFlag_EOS(priv->ADCx);
        LL_ADC_EnableIT_EOS(priv->ADCx);

        if (old_mode == ADC_OPMODE_UNINIT) {
            // Nothing is started yet - this is the only way to leave UNINIT
            LL_ADC_Enable(priv->ADCx);
            LL_DMA_EnableChannel(priv->DMAx, priv->dma_chnum);
            LL_TIM_EnableCounter(priv->TIMx);

            LL_ADC_REG_StartConversion(priv->ADCx);
        }
    }
    else if (new_mode == ADC_OPMODE_EMERGENCY_SHUTDOWN) {
        adc_dbg("ADC switch -> EMERGENCY_STOP");
        // Emergency shutdown is used when the job queue overflows and the stream is torn
        // This however doesn't help in the case when user sets such a high frequency
        // that the whole app becomes unresponsive due to the completion ISR, need to verify the value manually.

        LL_DMA_ClearFlag_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_ClearFlag_TC(priv->DMAx, priv->dma_chnum);
        LL_DMA_DisableIT_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_DisableIT_TC(priv->DMAx, priv->dma_chnum);

        LL_TIM_DisableCounter(priv->TIMx);
        UADC_SetSampleRate(unit, 10000); // fallback to a known safe value

        LL_ADC_ClearFlag_EOS(priv->ADCx);
        LL_ADC_DisableIT_EOS(priv->ADCx);

        unit->tick_interval = 0;
        unit->_tick_cnt = 250; // 1-off
    }
    else if (new_mode == ADC_OPMODE_ARMED) {
        adc_dbg("ADC switch -> ARMED");
        assert_param(old_mode == ADC_OPMODE_IDLE || old_mode == ADC_OPMODE_REARM_PENDING);

        // avoid firing immediately by the value jumping across the scale
        priv->trig_prev_level = priv->last_samples[priv->trigger_source];
    }
    else if (new_mode == ADC_OPMODE_TRIGD || new_mode == ADC_OPMODE_STREAM || new_mode == ADC_OPMODE_BLCAP) {
        adc_dbg("ADC switch -> CAPTURE");

        assert_param(old_mode == ADC_OPMODE_ARMED || old_mode == ADC_OPMODE_IDLE);

        // during the capture, we disallow direct readout and averaging to reduce overhead
        LL_ADC_DisableIT_EOS(priv->ADCx);

        // Enable the DMA buffer interrupts

        // we must first clear the flags, otherwise it will cause WEIRD bugs in the handler
        LL_DMA_ClearFlag_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_ClearFlag_TC(priv->DMAx, priv->dma_chnum);

        // those must be as close as possible to the enabling
        // if not trig'd, we don't care for lost samples before (this could cause a DMA irq miss / ht/tc mismatch with the startpos)
        if (new_mode != ADC_OPMODE_TRIGD) {
            priv->stream_startpos = DMA_POS(priv);
            priv->stream_serial = 0;
        }
        LL_DMA_EnableIT_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_EnableIT_TC(priv->DMAx, priv->dma_chnum);
    }

    priv->opmode = new_mode;
}
