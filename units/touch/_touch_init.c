//
// Created by MightyPork on 2018/02/03.
//

#include <stm32f072xb.h>
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

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t UTOUCH_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    TRY(rsc_claim(unit, R_TSC));

    // enable clock
    hw_periph_clock_enable(TSC);

    for (int gi = 0; gi < 8; gi++) {
        const uint8_t cap = priv->cfg.group_scaps[gi];
        const uint8_t ch = priv->cfg.group_channels[gi];

        if (cap == 0) {
            if (ch != 0) {
                dbg("TSC group %d has no cap!", (int)(gi+1));
                return E_BAD_CONFIG;
            }
            continue;
        }

        if (ch == 0) continue; // if no channels, don't bother setting up anything

        if (cap != 2 && cap != 4 && cap != 8 && cap != 16) {
            dbg("TSC group %d has more than 1 cap!", (int)(gi+1));
            return E_BAD_CONFIG;
        }

        if (cap & ch) {
            dbg("TSC pin can't be both channel and cap! (gpr %d)", (int)(gi+1));
            return E_BAD_CONFIG;
        }

        int chnum = 0;
        for (int pi = 0; pi < 4; pi++) {
            // pin numbers are 1-based in the config
            const bool iscap = 0 != (cap & (2 << pi));
            const bool isch = 0 != (ch & (2 << pi));

            if (iscap || isch) {
                Resource r = utouch_group_rscs[gi][pi];
                TRY(rsc_claim(unit, r));
                // 7 and 8 (1-based) use AF1, else AF3
                TRY(hw_configure_gpiorsc_af(r, gi >= 6 ? LL_GPIO_AF_1 : LL_GPIO_AF_3));

                if (iscap) dbg("TSC *cap @ %s", rsc_get_name(r));
                else dbg("TSC -ch @ %s", rsc_get_name(r));
            }

            // Sampling cap
            if (iscap) {
                TSC->IOSCR |= (cap>>1) << (gi*4);
            }

            // channels are configured individually when read.
            // we prepare bitmaps to use for the read groups (all can be read in at most 3 steps)
            if (isch) {
                priv->channels_phase[chnum] |= 1 << (gi*4+pi);
                priv->pgen_phase[chnum] |= 1 << gi;
                chnum++;
            }
        }
    }

    dbg("TSC phases:");
    for (int i = 0; i < 3; i++) {
        dbg(" %d: ch %08"PRIx32", g %02"PRIx32, i+1, priv->channels_phase[i], (uint32_t)priv->pgen_phase[i]);
    }

    return E_SUCCESS;
}


/** Tear down the unit */
void UTOUCH_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init peripherals
    if (unit->status == E_SUCCESS ) {
        hw_periph_clock_disable(TSC);
        // clear all registers to their default values
        __HAL_RCC_TSC_FORCE_RESET();
        __HAL_RCC_TSC_RELEASE_RESET();
    }

    // Release all resources, deinit pins
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
