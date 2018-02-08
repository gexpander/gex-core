//
// Created by MightyPork on 2018/02/04.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_adc.h"

#define ADC_INTERNAL
#include "_adc_internal.h"

#define DMA_POS(priv) ((priv)->dma_buffer_itemcount - (priv)->DMA_CHx->CNDTR)

static void UADC_JobSendBlockChunk(Job *job)
{
    Unit *unit = job->unit;
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    uint32_t start = job->data1;
    uint32_t count = job->data2;
    bool close = (bool) (job->data3 & 0x80);
    bool tc = (bool) (job->data3 & 0x01);

//    dbg("Send indices [%d -> %d)", (int)start, (int)(start+count));

    TF_TYPE type = close ? EVT_CAPT_DONE : EVT_CAPT_MORE;

    TF_Msg msg = {
        .frame_id = priv->stream_frame_id,
        .len = (TF_LEN) (1 + count*sizeof(uint16_t)),
        .type = type,
    };

    TF_Respond_Multipart(comm, &msg);
    TF_Multipart_Payload(comm, &priv->stream_serial, 1);
    TF_Multipart_Payload(comm, (uint8_t *) (priv->dma_buffer + start), count * sizeof(uint16_t));
    TF_Multipart_Close(comm);

    if (tc) priv->tc_pending = false;
    else priv->ht_pending = false;

    priv->stream_serial++;
}

static void UADC_JobSendTriggerCaptureHeader(Job *job)
{
    Unit *unit = job->unit;
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    EventReport er = {
        .unit = unit,
        .type = EVT_CAPT_START,
        .timestamp = job->timestamp,
        .length = (priv->pretrig_len+1)*priv->nb_channels*sizeof(uint16_t) + 2 /*pretrig len*/ + 1 /*edge*/ + 1 /* seq */
    };

    uint16_t index_trigd = (uint16_t) job->data1;
    uint8_t edge = (uint8_t) job->data2;

    EventReport_Start(&er);
    priv->stream_frame_id = er.sent_msg_id;
//    dbg("Sending TRIG HEADER with id %d (idx %d)", (int)er.sent_msg_id, (int)index_trigd);
    {
        // preamble
        uint8_t buf[4];
        PayloadBuilder pb = pb_start(buf, 4, NULL);
        pb_u16(&pb, priv->pretrig_len);
        pb_u8(&pb, edge);
        pb_u8(&pb, priv->stream_serial++); // This is the serial counter for the first chunk
                       // (containing the pre-trigger, or empty if no pretrig configured)
        EventReport_PB(&pb);

        if (priv->pretrig_len > 0) {
            // pretrig
            uint16_t pretrig_remain = (uint16_t) ((priv->pretrig_len + 1) * priv->nb_channels); // +1 because we want pretrig 0 to exactly start with the triggering sample

            assert_param(index_trigd <= priv->dma_buffer_itemcount);

            // this is one past the last entry of the triggering capture group
            if (pretrig_remain > index_trigd) {
                // used items in the wrap-around part of the buffer
                uint16_t items_from_end = pretrig_remain - index_trigd;
                assert_param(priv->dma_buffer_itemcount - items_from_end >= index_trigd);

//                dbg("Pretrig wraparound part: start %d, len %d",
//                    (int) (priv->dma_buffer_itemcount - items_from_end),
//                    (int) items_from_end
//                );

                EventReport_Data(
                    (uint8_t *) &priv->dma_buffer[priv->dma_buffer_itemcount -
                                                  items_from_end],
                    items_from_end * sizeof(uint16_t));

                assert_param(items_from_end <= pretrig_remain);
                pretrig_remain -= items_from_end;
            }

//            dbg("Pretrig front part: start %d, len %d",
//                (int) (index_trigd - pretrig_remain),
//                (int) pretrig_remain
//            );

            assert_param(pretrig_remain <= index_trigd);
            EventReport_Data((uint8_t *) &priv->dma_buffer[index_trigd - pretrig_remain],
                             pretrig_remain * sizeof(uint16_t));
        }
    }
    EventReport_End();
}

static void UADC_JobSendEndOfStreamMsg(Job *job)
{
    Unit *unit = job->unit;
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    TF_Msg msg = {
        .type = EVT_CAPT_DONE,
        .frame_id = (TF_ID) job->data1
    };
    TF_Respond(comm, &msg);
}

void UADC_ReportEndOfStream(Unit *unit)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    Job j = {
        .unit = unit,
        .data1 = priv->stream_frame_id,
        .cb = UADC_JobSendEndOfStreamMsg
    };
    scheduleJob(&j);
}

static void handle_httc(Unit *unit, bool tc)
{
    struct priv *priv = unit->data;
    uint16_t start = priv->stream_startpos;
    uint16_t end;
    const bool ht = !tc;

    const bool m_trigd = priv->opmode == ADC_OPMODE_TRIGD;
    const bool m_stream = priv->opmode == ADC_OPMODE_STREAM;
    const bool m_fixcpt = priv->opmode == ADC_OPMODE_BLCAP;

    if (ht) {
//                    dbg("HT");
        end = (uint16_t) (priv->dma_buffer_itemcount / 2);
        LL_DMA_ClearFlag_HT(priv->DMAx, priv->dma_chnum);
    }
    else {
//                    dbg("TC");
        end = (uint16_t) priv->dma_buffer_itemcount;
        LL_DMA_ClearFlag_TC(priv->DMAx, priv->dma_chnum);
    }

    if (ht == tc) {
        // This shouldn't happen - looks like we missed the TC flag
        dbg("!! %d -> %d", (int) start, (int) end);
        // TODO we could try to catch up. for now, just take what is easy to grab and hope it doesnt matter
        if (end == 64) start = 0;
    }

    if (start != end) {
        uint32_t sgcount = (end - start) / priv->nb_channels;

        if (m_trigd || m_fixcpt) {
            sgcount = MIN(priv->trig_stream_remain, sgcount);
            priv->trig_stream_remain -= sgcount;
        }

        bool close = !m_stream && priv->trig_stream_remain == 0;

        if ((tc && priv->tc_pending) || (ht && priv->ht_pending)) {
            dbg("(!) DMA not handled in time, abort capture");
            UADC_SwitchMode(unit, ADC_OPMODE_EMERGENCY_SHUTDOWN);
            return;
        }

        Job j = {
            .unit = unit,
            .data1 = start,
            .data2 = sgcount * priv->nb_channels,
            .data3 = (uint32_t) (close*0x80) | (tc*1),
            .cb = UADC_JobSendBlockChunk
        };

        if (tc)
            priv->tc_pending = true;
        else
            priv->ht_pending = true;

        if (!scheduleJob(&j)) {
            // Abort if we can't queue - the stream would tear and we'd hog the system with error messages
            dbg("(!) Buffers overflow, abort capture");
            UADC_SwitchMode(unit, ADC_OPMODE_EMERGENCY_SHUTDOWN);
            return;
        }

        if (close) {
            // If auto-arm enabled, we need to re-arm again.
            // However, EOS irq is disabled during the capture.
            // We have to wait for the next EOS interrupt to occur.
            UADC_SwitchMode(unit, (priv->auto_rearm && m_trigd) ? ADC_OPMODE_REARM_PENDING : ADC_OPMODE_IDLE);
        }
    } else {
//                    dbg("start==end, skip this irq");
    }

    if (tc) {
        priv->stream_startpos = 0;
    }
    else {
        priv->stream_startpos = end;
    }
}

void UADC_DMA_Handler(void *arg)
{
    Unit *unit = arg;

    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    if (priv->opmode == ADC_OPMODE_UNINIT) {
        LL_DMA_ClearFlag_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_ClearFlag_TC(priv->DMAx, priv->dma_chnum);
        LL_DMA_ClearFlag_TE(priv->DMAx, priv->dma_chnum);
        return;
    }

    const uint32_t isrsnapshot = priv->DMAx->ISR;

    if (LL_DMA_IsActiveFlag_G(isrsnapshot, priv->dma_chnum)) {
        const bool tc = LL_DMA_IsActiveFlag_TC(isrsnapshot, priv->dma_chnum);
        const bool ht = LL_DMA_IsActiveFlag_HT(isrsnapshot, priv->dma_chnum);
        const bool te = LL_DMA_IsActiveFlag_TE(isrsnapshot, priv->dma_chnum);

        // check what mode we're in
        const bool m_trigd = priv->opmode == ADC_OPMODE_TRIGD;
        const bool m_stream = priv->opmode == ADC_OPMODE_STREAM;
        const bool m_fixcpt = priv->opmode == ADC_OPMODE_BLCAP;

        if (m_trigd || m_stream || m_fixcpt) {
            if (ht || tc) {
                if (ht && tc) {
                    uint16_t half = (uint16_t) (priv->dma_buffer_itemcount / 2);
                    if (priv->stream_startpos > half) {
                        handle_httc(unit, true);
                        handle_httc(unit, false);
                    } else {
                        handle_httc(unit, false);
                        handle_httc(unit, true);
                    }
                } else {
                    handle_httc(unit, tc);
                }
            }
        } else {
            // This shouldn't happen, the interrupt should be disabled in this opmode
            dbg("(!) not streaming, DMA IT should be disabled");

            if (ht) {
                LL_DMA_ClearFlag_HT(priv->DMAx, priv->dma_chnum);
            }
            else {
                LL_DMA_ClearFlag_TC(priv->DMAx, priv->dma_chnum);
            }
        }

        if (te) {
            // this shouldn't happen - error
            dbg("ADC DMA TE!");
            LL_DMA_ClearFlag_TE(priv->DMAx, priv->dma_chnum);
        }
    }
}

void UADC_ADC_EOS_Handler(void *arg)
{
    Unit *unit = arg;
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    // Normally
    uint64_t timestamp = 0;
    if (priv->opmode == ADC_OPMODE_ARMED) timestamp = PTIM_GetMicrotime();

    LL_ADC_ClearFlag_EOS(priv->ADCx);
    if (priv->opmode == ADC_OPMODE_UNINIT) return;

    // Wait for the DMA to complete copying the last sample
    uint16_t dmapos;
    hw_wait_while((dmapos = (uint16_t) DMA_POS(priv)) % priv->nb_channels != 0, 100); // XXX this could be changed to reading it from the DR instead

    uint32_t sample_pos;
    if (dmapos == 0) {
        sample_pos = (uint32_t) (priv->dma_buffer_itemcount);
    } else {
        sample_pos = dmapos;
    }
    sample_pos -= priv->nb_channels;

    int cnt = 0; // index of the sample within the group

    const bool can_average = priv->real_frequency_int < UADC_MAX_FREQ_FOR_AVERAGING;
    const uint32_t channels_mask = priv->extended_channels_mask;

    for (uint8_t i = 0; i < 18; i++) {
        if (channels_mask & (1 << i)) {
            uint16_t val = priv->dma_buffer[sample_pos+cnt];
            cnt++;

            if (can_average) {
                priv->averaging_bins[i] =
                    priv->averaging_bins[i] * (1.0f - priv->avg_factor_as_float) +
                    ((float) val) * priv->avg_factor_as_float;
            }

            priv->last_samples[i] = val;
        }
    }

    if (priv->opmode == ADC_OPMODE_ARMED) {
        uint16_t val =  priv->last_samples[priv->trigger_source];

//        dbg("Trig line level %d", (int)val);
        if ((priv->trig_prev_level < priv->trig_level) && val >= priv->trig_level && (bool) (priv->trig_edge & 0b01)) {
//            dbg("******** Rising edge");
            // Rising edge
            UADC_HandleTrigger(unit, 1, timestamp);
        }
        else if ((priv->trig_prev_level > priv->trig_level) && val <= priv->trig_level && (bool) (priv->trig_edge & 0b10)) {
//            dbg("******** Falling edge");
            // Falling edge
            UADC_HandleTrigger(unit, 2, timestamp);
        }
        priv->trig_prev_level = val;
    }

    // auto-rearm was waiting for the next sample
    if (priv->opmode == ADC_OPMODE_REARM_PENDING) {
        if (!priv->auto_rearm) {
            // It looks like the flag was cleared by DISARM before we got a new sample.
            // Let's just switch to IDLE
            UADC_SwitchMode(unit, ADC_OPMODE_IDLE);
        } else {
            // Re-arming for a new trigger
            UADC_SwitchMode(unit, ADC_OPMODE_ARMED);
        }
    }
}

void UADC_HandleTrigger(Unit *unit, uint8_t edge_type, uint64_t timestamp)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);
    if (priv->opmode == ADC_OPMODE_UNINIT) return;

    if (priv->trig_holdoff != 0 && priv->trig_holdoff_remain > 0) {
//        dbg("Trig discarded due to holdoff.");
        return;
    }

    if (priv->trig_holdoff > 0) {
        priv->trig_holdoff_remain = priv->trig_holdoff;
        // Start the tick
        unit->tick_interval = 1;
        unit->_tick_cnt = 1;
    }

    priv->stream_startpos = (uint16_t) DMA_POS(priv);
    priv->trig_stream_remain = priv->trig_len;
    priv->stream_serial = 0;

//    dbg("Trigger condition hit, edge=%d, startpos %d", edge_type, (int)priv->stream_startpos);

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

void UADC_StartBlockCapture(Unit *unit, uint32_t len, TF_ID frame_id)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);
    if (priv->opmode == ADC_OPMODE_UNINIT) return;

    priv->stream_frame_id = frame_id;
    priv->stream_startpos = (uint16_t) DMA_POS(priv);
    priv->trig_stream_remain = len;
    priv->stream_serial = 0;
    UADC_SwitchMode(unit, ADC_OPMODE_BLCAP);
}

/** Start stream */
void UADC_StartStream(Unit *unit, TF_ID frame_id)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);
    if (priv->opmode == ADC_OPMODE_UNINIT) return;

    priv->stream_frame_id = frame_id;
    priv->stream_startpos = (uint16_t) DMA_POS(priv);
    priv->stream_serial = 0;
    UADC_SwitchMode(unit, ADC_OPMODE_STREAM);
}

/** End stream */
void UADC_StopStream(Unit *unit)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);
    if (priv->opmode == ADC_OPMODE_UNINIT) return;

    UADC_ReportEndOfStream(unit);
    UADC_SwitchMode(unit, ADC_OPMODE_IDLE);
}

/** Handle unit update tick - expire the trigger hold-off */
void UADC_updateTick(Unit *unit)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    // Recover from shutdown after a delay
    if (priv->opmode == ADC_OPMODE_EMERGENCY_SHUTDOWN) {
        dbg("Recovering from emergency shutdown");
        UADC_SwitchMode(unit, ADC_OPMODE_IDLE);
        LL_TIM_EnableCounter(priv->TIMx);
        UADC_ReportEndOfStream(unit);
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

void UADC_SwitchMode(Unit *unit, enum uadc_opmode new_mode)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    const enum uadc_opmode old_mode = priv->opmode;

    if (new_mode == old_mode) return; // nothing to do

    // if un-itied, can go only to IDLE
    assert_param((old_mode != ADC_OPMODE_UNINIT) || (new_mode == ADC_OPMODE_IDLE));

    priv->opmode = ADC_OPMODE_UNINIT;

    if (new_mode == ADC_OPMODE_UNINIT) {
//        dbg("ADC switch -> UNINIT");
        // Stop the DMA, timer and disable ADC - this is called before tearing down the unit
        LL_TIM_DisableCounter(priv->TIMx);

        // Switch off the ADC
        if (LL_ADC_IsEnabled(priv->ADCx)) {
            // Cancel ongoing conversion
            if (LL_ADC_REG_IsConversionOngoing(priv->ADCx)) {
//                dbg("Stopping ADC conv");
                LL_ADC_REG_StopConversion(priv->ADCx);
                hw_wait_while(LL_ADC_REG_IsStopConversionOngoing(priv->ADCx), 100);
            }

            LL_ADC_Disable(priv->ADCx);
//            dbg("Disabling ADC");
            hw_wait_while(LL_ADC_IsDisableOngoing(priv->ADCx), 100);
        }

//        dbg("Disabling DMA");
        LL_DMA_DisableChannel(priv->DMAx, priv->dma_chnum);
        LL_DMA_DisableIT_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_DisableIT_TC(priv->DMAx, priv->dma_chnum);
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
//        dbg("ADC switch -> ARMED");
        assert_param(old_mode == ADC_OPMODE_IDLE || old_mode == ADC_OPMODE_REARM_PENDING);

        // avoid firing immediately by the value jumping across the scale
        priv->trig_prev_level = priv->last_samples[priv->trigger_source];
    }
    else if (new_mode == ADC_OPMODE_TRIGD ||
        new_mode == ADC_OPMODE_STREAM ||
        new_mode == ADC_OPMODE_BLCAP) {

//        dbg("ADC switch -> TRIG'D / STREAM / BLOCK");
        assert_param(old_mode == ADC_OPMODE_ARMED || old_mode == ADC_OPMODE_IDLE);

        // during the capture, we disallow direct readout and averaging to reduce overhead
        LL_ADC_DisableIT_EOS(priv->ADCx);

        // Enable the DMA buffer interrupts

        // we must first clear the flags, otherwise it will cause WEIRD bugs in the handler
        LL_DMA_ClearFlag_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_ClearFlag_TC(priv->DMAx, priv->dma_chnum);

        LL_DMA_EnableIT_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_EnableIT_TC(priv->DMAx, priv->dma_chnum);
    }

    priv->opmode = new_mode;
}
