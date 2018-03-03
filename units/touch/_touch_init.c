//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define TOUCH_INTERNAL

#include "_touch_internal.h"

/** Allocate data structure and set defaults */
error_t UTOUCH_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    priv->cfg.charge_time = 2;
    priv->cfg.drain_time = 2;
    priv->cfg.spread_deviation = 0;
    priv->cfg.ss_presc = 1;
    priv->cfg.pg_presc = 32;
    priv->cfg.sense_timeout = 7;
    memset(priv->cfg.group_scaps, 0, 8);
    memset(priv->cfg.group_channels, 0, 8);
    priv->cfg.binary_hysteresis = 10;
    priv->cfg.binary_debounce_ms = 20;

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t UTOUCH_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    unit->tick_interval = 1; // sample every 1 ms

    // copy from conf
    priv->binary_debounce_ms = priv->cfg.binary_debounce_ms;
    priv->binary_hysteresis = priv->cfg.binary_hysteresis;

    TRY(rsc_claim(unit, R_TSC));

    // simple bound checks, just clamp without error
    if (priv->cfg.charge_time > 16) priv->cfg.charge_time = 16;
    if (priv->cfg.charge_time < 1) priv->cfg.charge_time = 1;

    if (priv->cfg.drain_time > 16) priv->cfg.drain_time = 16;
    if (priv->cfg.drain_time < 1) priv->cfg.drain_time = 1;

    if (priv->cfg.spread_deviation > 128) priv->cfg.drain_time = 128;

    if (priv->cfg.ss_presc > 2) priv->cfg.ss_presc = 2;
    if (priv->cfg.ss_presc < 1) priv->cfg.ss_presc = 1;

    if (priv->cfg.sense_timeout > 7) priv->cfg.sense_timeout = 7;
    if (priv->cfg.sense_timeout < 1) priv->cfg.sense_timeout = 1;

    uint8_t tmppgpresc = priv->cfg.pg_presc;
    if (tmppgpresc == 0) return E_BAD_CONFIG;
    uint8_t pgpresc_reg = 0;
    while ((tmppgpresc & 1) == 0 && tmppgpresc != 0) {
        pgpresc_reg++;
        tmppgpresc >>= 1;
    }
    if (tmppgpresc != 1 || pgpresc_reg > 7) {
        dbg("Bad pgpresc");
        return E_BAD_CONFIG; // TODO better reporting
    }

    if ((pgpresc_reg==0 && priv->cfg.drain_time<=2) || (pgpresc_reg==1 && priv->cfg.drain_time==0)) {
        dbg("Illegal PGPSC vs CTPL");
        return E_BAD_CONFIG;
    }

    // enable clock
    hw_periph_clock_enable(TSC);
    // reset
    __HAL_RCC_TSC_FORCE_RESET();
    __HAL_RCC_TSC_RELEASE_RESET();

    priv->all_channels_mask = 0;

    for (int gi = 0; gi < 8; gi++) {
        const uint8_t cap = priv->cfg.group_scaps[gi];
        const uint8_t ch = priv->cfg.group_channels[gi];

        if (cap == 0) {
            if (ch != 0) {
                dbg_touch("TSC group %d has no cap!", (int) (gi + 1));
                return E_BAD_CONFIG;
            }
            continue;
        }

        if (ch == 0) continue; // if no channels, don't bother setting up anything

        if (cap != 2 && cap != 4 && cap != 8 && cap != 16) {
            dbg_touch("TSC group %d has more than 1 cap!", (int) (gi + 1));
            return E_BAD_CONFIG;
        }

        if (cap & ch) {
            dbg_touch("TSC pin can't be both channel and cap! (gpr %d)", (int) (gi + 1));
            return E_BAD_CONFIG;
        }

        // This is a loop through the pins in a group gi
        int phasenum = 0;
        for (int pi = 0; pi < 4; pi++) {
            // pin numbers are 1-based in the config
            const bool iscap = 0 != (cap & (2 << pi));
            const bool isch = 0 != (ch & (2 << pi));

            if (!iscap && !isch) continue;

            Resource r = utouch_group_rscs[gi][pi];
            TRY(rsc_claim(unit, r));

            GPIO_TypeDef *port;
            uint32_t ll;
            assert_param(hw_pinrsc2ll(r, &port, &ll));
            LL_GPIO_SetPinOutputType(port, ll, isch ? LL_GPIO_OUTPUT_PUSHPULL : LL_GPIO_OUTPUT_OPENDRAIN);

            // 7 and 8 (1-based) use AF1, else AF3
            TRY(hw_configure_gpiorsc_af(r, gi >= 6 ? LL_GPIO_AF_1 : LL_GPIO_AF_3));

            uint32_t bit = (uint32_t) (1 << (gi * 4 + pi));

            if (iscap) {
                dbg_touch("TSC cap @ %s", rsc_get_name(r));
                // Sampling cap
                TSC->IOSCR |= bit;

                // Disable pin hysteresis (causes noise)
                TSC->IOHCR ^= bit;
            }
            else {
                dbg_touch("TSC ch @ %s", rsc_get_name(r));

                if (priv->cfg.interlaced) {
                    // interlaced - only update the mask beforehand
                    priv->all_channels_mask |= bit;
                } else {
                    // channels are configured individually when read.
                    // we prepare bitmaps to use for the read groups (all can be read in at most 3 steps)
                    priv->channels_phase[phasenum] |= bit; // this is used for the channel selection register
                    priv->groups_phase[phasenum] |= 1 << gi; // this will be used for the group enable register, if all 0, this and any following phases are unused.
                    phasenum++;
                }
            }
        }
    }

    // common TSC config
    TSC->CR =
        ((priv->cfg.charge_time - 1) << TSC_CR_CTPH_Pos) |
        ((priv->cfg.drain_time - 1) << TSC_CR_CTPL_Pos) |
        ((priv->cfg.ss_presc - 1) << TSC_CR_SSPSC_Pos) |
        (pgpresc_reg << TSC_CR_PGPSC_Pos) |
        ((priv->cfg.sense_timeout - 1) << TSC_CR_MCV_Pos) |
        TSC_CR_TSCE;

    if (priv->cfg.spread_deviation > 0) {
        TSC->CR |= ((priv->cfg.spread_deviation - 1) << TSC_CR_SSD_Pos) | TSC_CR_SSE;
    }

    dbg_touch("CR = %08x, ht is %d, lt is %d", (int)TSC->CR,
              (int)priv->cfg.charge_time,
              (int)priv->cfg.drain_time);

    // iofloat is used for discharging

    // Enable the interrupts
    TSC->IER = TSC_IER_EOAIE | TSC_IER_MCEIE;
    irqd_attach(TSC, UTOUCH_HandleIrq, unit);

    if (!priv->cfg.interlaced) {
        dbg_touch("TSC phases:");
        for (int i = 0; i < 3; i++) {
            priv->all_channels_mask |= priv->channels_phase[i];

            dbg_touch(" %d: ch %08"PRIx32", g %02"PRIx32,
                      i + 1,
                      priv->channels_phase[i],
                      (uint32_t) priv->groups_phase[i]);
        }
    }

    priv->status = UTSC_STATUS_BUSY; // first loop ...
    priv->next_phase = 0;

    // starts in the tick callback

    return E_SUCCESS;
}


/** Tear down the unit */
void UTOUCH_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init peripherals
    if (unit->status == E_SUCCESS) {
        hw_periph_clock_disable(TSC);
        // clear all registers to their default values
        __HAL_RCC_TSC_FORCE_RESET();
        __HAL_RCC_TSC_RELEASE_RESET();

        irqd_detach(TSC, UTOUCH_HandleIrq);
    }

    // Release all resources, deinit pins
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
