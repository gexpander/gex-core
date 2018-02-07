//
// Created by MightyPork on 2018/02/04.
//

#include "platform.h"
#include "unit_base.h"

#define ADC_INTERNAL
#include "_adc_internal.h"

#define DMA_POS(priv) ((priv)->dma_buffer_itemcount - (priv)->DMA_CHx->CNDTR)

void UADC_ReportEndOfStream(Unit *unit)
{
    dbg("~~End Of Stream msg~~");
}

void UADC_DMA_Handler(void *arg)
{
    Unit *unit = arg;

    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    const uint32_t isrsnapshot = priv->DMAx->ISR;

    if (LL_DMA_IsActiveFlag_G(isrsnapshot, priv->dma_chnum)) {
        const bool tc = LL_DMA_IsActiveFlag_TC(isrsnapshot, priv->dma_chnum);
        const bool ht = LL_DMA_IsActiveFlag_HT(isrsnapshot, priv->dma_chnum);
        const bool te = LL_DMA_IsActiveFlag_TE(isrsnapshot, priv->dma_chnum);

        // check what mode we're in
        const bool m_trigd = priv->opmode == ADC_OPMODE_TRIGD;
        const bool m_stream = priv->opmode == ADC_OPMODE_STREAM;
        const bool m_fixcpt = priv->opmode == ADC_OPMODE_FIXCAPT;

        if (m_trigd || m_stream || m_fixcpt) {
            if (ht || tc) {
                const uint16_t start = priv->stream_startpos;
                uint16_t end;

                if (ht) {
                    dbg("HT");
                    end = (uint16_t) (priv->dma_buffer_itemcount / 2);
                    LL_DMA_ClearFlag_HT(priv->DMAx, priv->dma_chnum);
                }
                else {
                    dbg("TC");
                    end = (uint16_t) priv->dma_buffer_itemcount;
                    LL_DMA_ClearFlag_TC(priv->DMAx, priv->dma_chnum);
                }

                dbg("start %d, end %d", (int)start, (int)end);
                assert_param(start <= end);

                if (start != end) {
                    uint32_t sgcount = (end - start) / priv->nb_channels;

                    if (m_trigd || m_fixcpt) {
                        sgcount = MIN(priv->trig_stream_remain, sgcount);
                        priv->trig_stream_remain -= sgcount;
                    }

                    dbg("Would send %d groups (u16 offset %d -> %d)", (int) sgcount,
                        (int) start, (int) (start + sgcount * priv->nb_channels));

                    // TODO send the data together with remaining count (used to detect end of transmission)

                    if (m_trigd || m_fixcpt) { // Trig'd or Block capture - check for the max count condition
                        if (priv->trig_stream_remain == 0) {
                            dbg("End of capture");
                            UADC_ReportEndOfStream(unit);

                            // If auto-arm enabled, we need to re-arm again.
                            // However, EOS irq is disabled during the capture.
                            // We have to wait for the next EOS interrupt to occur.
                            // TODO verify if keeping the EOS irq enabled during capture has significant performance penalty. If not, we can leave it enabled.
                            UADC_SwitchMode(unit, (priv->auto_rearm && m_trigd) ? ADC_OPMODE_REARM_PENDING : ADC_OPMODE_IDLE);
                        }
                    }
                } else {
                    dbg("start==end, skip this irq");
                }

                if (tc) {
                    priv->stream_startpos = 0;
                }
                else {
                    priv->stream_startpos = end;
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
    uint64_t timestamp = PTIM_GetMicrotime();
    Unit *unit = arg;

    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    LL_ADC_ClearFlag_EOS(priv->ADCx);

    // Wait for the DMA to complete copying the last sample
    uint16_t dmapos;
    while ((dmapos = (uint16_t) DMA_POS(priv)) % priv->nb_channels != 0);

    uint32_t sample_pos;
    if (dmapos == 0) {
        sample_pos = (uint32_t) (priv->dma_buffer_itemcount);
    } else {
        sample_pos = dmapos;
    }
    sample_pos -= priv->nb_channels;

    int cnt = 0; // index of the sample within the group
    for (uint32_t i = 0; i < 18; i++) {
        if (priv->extended_channels_mask & (1 << i)) {
            uint16_t val = priv->dma_buffer[sample_pos+cnt];
            cnt++;

            priv->averaging_bins[i] =
                priv->averaging_bins[i] * (1.0f - priv->avg_factor_as_float) +
                ((float) val) * priv->avg_factor_as_float;

            priv->last_samples[i] = val;

            if (i == priv->trigger_source) {
                if (priv->opmode == ADC_OPMODE_ARMED) {
                    dbg("Trig line level %d", (int)val);

                    bool trigd = false;
                    uint8_t edge_type = 0;
                    if (priv->trig_prev_level < priv->trig_level && val >= priv->trig_level) {
                        dbg("******** Rising edge");
                        // Rising edge
                        trigd = (bool) (priv->trig_edge & 0b01);
                        edge_type = 1;
                    }
                    else if (priv->trig_prev_level > priv->trig_level && val <= priv->trig_level) {
                        dbg("******** Falling edge");
                        // Falling edge
                        trigd = (bool) (priv->trig_edge & 0b10);
                        edge_type = 2;
                    }

                    if (trigd) {
                        UADC_HandleTrigger(unit, edge_type, timestamp);
                    }
                }
                else if (priv->opmode == ADC_OPMODE_REARM_PENDING) {
                    if (!priv->auto_rearm) {
                        // It looks like the flag was cleared by DISARM before we got a new sample.
                        // Let's just switch to IDLE
                        UADC_SwitchMode(unit, ADC_OPMODE_IDLE);
                    } else {
                        // Re-arming for a new trigger
                        UADC_SwitchMode(unit, ADC_OPMODE_ARMED);
                    }
                }

                priv->trig_prev_level = val;
            }
        }
    }
}

void UADC_HandleTrigger(Unit *unit, uint8_t edge_type, uint64_t timestamp)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    if (priv->trig_holdoff != 0 && priv->trig_holdoff_remain > 0) {
        dbg("Trig discarded due to holdoff.");
        return;
    }

    if (priv->trig_holdoff > 0) {
        priv->trig_holdoff_remain = priv->trig_holdoff;
        // Start the tick
        unit->tick_interval = 1;
        unit->_tick_cnt = 0;
    }

    // TODO Send pre-trigger

    priv->stream_startpos = (uint16_t) DMA_POS(priv);
    priv->trig_stream_remain = priv->trig_len;

    dbg("Trigger condition hit, edge=%d, startpos %d", edge_type, (int)priv->stream_startpos);

    UADC_SwitchMode(unit, ADC_OPMODE_TRIGD);
}

void UADC_StartBlockCapture(Unit *unit, uint32_t len, TF_ID frame_id)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    priv->stream_frame_id = frame_id;
    priv->stream_startpos = (uint16_t) DMA_POS(priv);
    priv->trig_stream_remain = len;
    UADC_SwitchMode(unit, ADC_OPMODE_FIXCAPT);
}

/** Start stream */
void UADC_StartStream(Unit *unit, TF_ID frame_id)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    priv->stream_frame_id = frame_id;
    priv->stream_startpos = (uint16_t) DMA_POS(priv);
    dbg("Start streaming.");
    UADC_SwitchMode(unit, ADC_OPMODE_STREAM);
}

/** End stream */
void UADC_StopStream(Unit *unit)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    dbg("Stop stream.");
    UADC_ReportEndOfStream(unit);
    UADC_SwitchMode(unit, ADC_OPMODE_IDLE);
}

/** Handle unit update tick - expire the trigger hold-off */
void UADC_updateTick(Unit *unit)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

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

    if (new_mode == priv->opmode) return; // nothing to do

    // if un-itied, can go only to IDLE
    assert_param((priv->opmode != ADC_OPMODE_UNINIT) || (new_mode == ADC_OPMODE_IDLE));

    if (new_mode == ADC_OPMODE_UNINIT) {
        dbg("ADC switch -> UNINIT");
        // Stop the DMA, timer and disable ADC - this is called before tearing down the unit
        LL_TIM_DisableCounter(priv->TIMx);

        // Switch off the ADC
        if (LL_ADC_IsEnabled(priv->ADCx)) {
            // Cancel ongoing conversion
            if (LL_ADC_REG_IsConversionOngoing(priv->ADCx)) {
                dbg("Stopping ADC conv");
                LL_ADC_REG_StopConversion(priv->ADCx);
                hw_wait_while(LL_ADC_REG_IsStopConversionOngoing(priv->ADCx), 100);
            }

            LL_ADC_Disable(priv->ADCx);
            dbg("Disabling ADC");
            hw_wait_while(LL_ADC_IsDisableOngoing(priv->ADCx), 100);
        }

        dbg("Disabling DMA");
        LL_DMA_DisableChannel(priv->DMAx, priv->dma_chnum);
        LL_DMA_DisableIT_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_DisableIT_TC(priv->DMAx, priv->dma_chnum);
    }
    else if (new_mode == ADC_OPMODE_IDLE || new_mode == ADC_OPMODE_REARM_PENDING) {
        dbg("ADC switch -> IDLE or IDLE/REARM_PENDING");
        // IDLE and ARMED are identical with the exception that the trigger condition is not checked
        // ARMED can be only entered from IDLE, thus we do the init only here.

        // In IDLE, we don't need the DMA interrupts
        LL_DMA_DisableIT_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_DisableIT_TC(priv->DMAx, priv->dma_chnum);

        // Use End Of Sequence to recover results for averaging from the DMA buffer and DR
        LL_ADC_ClearFlag_EOS(priv->ADCx);
        LL_ADC_EnableIT_EOS(priv->ADCx);

        if (priv->opmode == ADC_OPMODE_UNINIT) {
            // Nothing is started yet - this is the only way to leave UNINIT
            LL_ADC_Enable(priv->ADCx);
            LL_DMA_EnableChannel(priv->DMAx, priv->dma_chnum);
            LL_TIM_EnableCounter(priv->TIMx);

            LL_ADC_REG_StartConversion(priv->ADCx);
        }
    }
    else if (new_mode == ADC_OPMODE_ARMED) {
        dbg("ADC switch -> ARMED");
        assert_param(priv->opmode == ADC_OPMODE_IDLE || priv->opmode == ADC_OPMODE_REARM_PENDING);

        // avoid firing immediately by the value jumping across the scale
        priv->trig_prev_level = priv->last_samples[priv->trigger_source];
    }
    else if (new_mode == ADC_OPMODE_TRIGD ||
        new_mode == ADC_OPMODE_STREAM ||
        new_mode == ADC_OPMODE_FIXCAPT) {

        dbg("ADC switch -> TRIG'D / STREAM / BLOCK");
        assert_param(priv->opmode == ADC_OPMODE_ARMED || priv->opmode == ADC_OPMODE_IDLE);

        // during the capture, we disallow direct readout and averaging to reduce overhead
        LL_ADC_DisableIT_EOS(priv->ADCx);

        // Enable the DMA buffer interrupts

        // we must first clear the flags, otherwise it will cause WEIRD bugs in the handler
        LL_DMA_ClearFlag_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_ClearFlag_TC(priv->DMAx, priv->dma_chnum);

        LL_DMA_EnableIT_HT(priv->DMAx, priv->dma_chnum);
        LL_DMA_EnableIT_TC(priv->DMAx, priv->dma_chnum);
    }

    // the actual switch
    priv->opmode = new_mode;
}
