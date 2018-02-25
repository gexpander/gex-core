//
// Created by MightyPork on 2018/02/25.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_touch.h"

#define TOUCH_INTERNAL

#include "_touch_internal.h"

#define DIS_TIME 1

static void startNextPhase(struct priv *priv);

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
        const uint32_t chmask = priv->channels_phase[priv->next_phase];
        for (int i = 0; i < 32; i++) {
            if (chmask & (1<<i)) {
                priv->readouts[i] = (uint16_t) (TSC->IOGXCR[i >> 2] & 0x3FFF);
            }
        }

        priv->next_phase++;
        // check if we've run out of existing or populated groups
        if (priv->next_phase == 3 || priv->groups_phase[priv->next_phase] == 0) {
            priv->next_phase = 0;
            priv->status = UTSC_STATUS_READY;
            // we'll stay in READY after the first loop until an error occurs or it's re-inited
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
        startNextPhase(priv);
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

static void startNextPhase(struct priv *priv)
{
    if (priv->next_phase == 0 && priv->groups_phase[0] == 0) {
        // no groups are configured
        return;
    }

    TSC->IOGCSR = priv->groups_phase[priv->next_phase];
    TSC->IOCCR = priv->channels_phase[priv->next_phase];
    TSC->ICR = TSC_ICR_EOAIC | TSC_ICR_MCEIC;

    if (priv->cfg.interlaced) {
        TSC->CR |= TSC_CR_IODEF;
        // floaty (must be used for interlaced pads)
        // note: interlaced pads also need to be sampled individually
    }

    // Go!
    priv->ongoing = true;
    TSC->CR |= TSC_CR_START;
}
