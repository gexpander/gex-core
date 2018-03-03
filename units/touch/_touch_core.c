//
// Created by MightyPork on 2018/02/25.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_touch.h"

#define TOUCH_INTERNAL

#include "_touch_internal.h"

// discharge time in ms
#define DIS_TIME 1

static void startNextPhase(Unit *unit);

static void UTOUCH_EventReportJob(Job *job)
{
    Unit *unit = job->unit;
    struct priv *priv = unit->data;

    uint8_t buf[8];
    PayloadBuilder pb = pb_start(buf, 8, NULL);
    pb_u32(&pb, pinmask_pack_32(~job->data1, priv->all_channels_mask)); // inverted and packed - all pins (pressed state)
    pb_u32(&pb, pinmask_pack_32(job->data2, priv->all_channels_mask)); // trigger generating pins
    assert_param(pb.ok);

    EventReport er = {
        .unit = unit,
        .type = 0x00,
        .length = 8,
        .data = buf,
        .timestamp = job->timestamp,
    };

    EventReport_Send(&er);
}

static void UTOUCH_CheckForBinaryEvents(Unit *const unit)
{
    struct priv *priv = unit->data;

    const uint32_t time_ms = PTIM_GetTime();

    if (priv->last_done_ms == 0) {
        // avoid bug with trigger on first capture
        priv->last_done_ms = time_ms;
    }

    const uint64_t ts = PTIM_GetMicrotime();
    uint32_t eventpins = 0;

    const uint16_t ms_elapsed = (uint16_t) (time_ms - priv->last_done_ms);

    for (uint16_t i = 0; i < 32; i++) {
        const uint32_t poke = (uint32_t) (1 << i);
        if (0 == (priv->all_channels_mask & poke)) continue;
        if (priv->binary_thr[i] == 0) continue; // skip disabled channels

        const bool isactive = (bool) (priv->binary_active_bits & poke);
        const bool can_go_up = !isactive && (priv->readouts[i] > (priv->binary_thr[i] + priv->binary_hysteresis));
        const bool can_go_down = isactive && (priv->readouts[i] < priv->binary_thr[i]);

        if (can_go_up) {
            priv->bin_trig_cnt[i] += ms_elapsed;
            if (priv->bin_trig_cnt[i] >= priv->binary_debounce_ms) {
                priv->binary_active_bits |= poke;
                priv->bin_trig_cnt[i] = 0; // reset for the other direction of the switch

                eventpins |= poke;
            }
        }
        else if (priv->bin_trig_cnt[i] > 0) {
            priv->bin_trig_cnt[i] = 0;
        }

        if (can_go_down) {
            priv->bin_trig_cnt[i] -= ms_elapsed;
            if (priv->bin_trig_cnt[i] <= -priv->binary_debounce_ms) {
                priv->binary_active_bits &= ~poke;
                priv->bin_trig_cnt[i] = 0; // reset for the other direction of the switch

                eventpins |= poke;
            }
        }
        else if (priv->bin_trig_cnt[i] < 0) {
            priv->bin_trig_cnt[i] = 0;
        }
    }

    if (eventpins != 0) {
        Job j = {
            .timestamp = ts,
            .data1 = priv->binary_active_bits,
            .data2 = eventpins,
            .unit = unit,
            .cb = UTOUCH_EventReportJob,
        };

        scheduleJob(&j);
    }

    priv->last_done_ms = time_ms;
}

void UTOUCH_HandleIrq(void *arg)
{
    Unit *unit = arg;
    struct priv *priv = unit->data;

    if (TSC->ISR & TSC_ISR_MCEF) {
        priv->status = UTSC_STATUS_FAIL;
        dbg_touch("TSC Failure.");
        TSC->ICR = TSC_ICR_EOAIC | TSC_ICR_MCEIC;
    }

    if (TSC->ISR & TSC_ISR_EOAF) {
        TSC->ICR = TSC_ICR_EOAIC;
//        assert_param((TSC->IOGCSR>>16) == priv->groups_phase[priv->next_phase]);

        // Store captured data
        const uint32_t chmask = TSC->IOCCR;
        for (int i = 0; i < 32; i++) {
            if (chmask & (1<<i)) {
                priv->readouts[i] = (uint16_t) (TSC->IOGXCR[i >> 2] & 0x3FFF);
            }
        }

        priv->next_phase++;

        if (!priv->cfg.interlaced) {
            // check if we've run out of existing or populated groups
            if (priv->next_phase == 3 || priv->groups_phase[priv->next_phase] == 0) {
                priv->next_phase = 0;
                priv->status = UTSC_STATUS_READY;
                UTOUCH_CheckForBinaryEvents(unit);
            }
        }

        TSC->CR &= ~TSC_CR_IODEF; // pull low - discharge
    }

    priv->ongoing = false;
    priv->discharge_delay = DIS_TIME;
}

#if TSC_DEBUG
static volatile uint32_t xcnt=0;
#endif

void UTOUCH_updateTick(Unit *unit)
{
#if TSC_DEBUG
    xcnt++;
#endif

    struct priv *priv = unit->data;

    if (priv->ongoing) {
        return;
    }

    if (priv->discharge_delay > 0) {
        priv->discharge_delay--;
    } else {
        startNextPhase(unit);
    }

#if TSC_DEBUG
    if(xcnt >= 250) {
        xcnt=0;
        PRINTF("> ");
        for (int i = 0; i < 32; i++) {
            if (priv->all_channels_mask & (1<<i)) {
                PRINTF("%d ", (int)priv->readouts[i]);
            }
        }
        PRINTF("\r\n");
    }
#endif
}

static void startNextPhase(Unit *unit)
{
    struct priv *priv = unit->data;

    if (priv->all_channels_mask == 0) return;

    if (priv->cfg.interlaced) {
        // Find the next non-zero bit, wrap around if needed
        while ((priv->all_channels_mask & (1<<priv->next_phase))==0) {
            priv->next_phase++;
            if (priv->next_phase == 32) {
                priv->next_phase = 0;
                priv->status = UTSC_STATUS_READY;
                UTOUCH_CheckForBinaryEvents(unit);
            }
        }
        TSC->IOGCSR = (uint32_t) (1 << (priv->next_phase >> 2)); // phase divided by 4
        TSC->IOCCR = (uint32_t) (1 << priv->next_phase);

        // interlaced - float neighbouring electrodes
        TSC->CR |= TSC_CR_IODEF;
    } else {
        TSC->IOGCSR = priv->groups_phase[priv->next_phase];
        TSC->IOCCR = priv->channels_phase[priv->next_phase];

        // separate - keep neighbouring electrodes at GND
    }

    TSC->ICR = TSC_ICR_EOAIC | TSC_ICR_MCEIC;

    // Go!
    priv->ongoing = true;
    TSC->CR |= TSC_CR_START;
}
