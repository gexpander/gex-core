//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define DIN_INTERNAL
#include "_din_internal.h"

/** Allocate data structure and set defaults */
error_t DIn_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    // some defaults
    priv->port_name = 'A';
    priv->pins = 0x0001;
    priv->pulldown = 0x0000;
    priv->pullup = 0x0000;

    priv->trig_rise = 0x0000;
    priv->trig_fall = 0x0000;
    priv->trig_holdoff = 100;
    priv->def_auto = 0x0000;

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t DIn_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    priv->pulldown &= priv->pins;
    priv->pullup &= priv->pins;
    priv->trig_rise &= priv->pins;
    priv->trig_fall &= priv->pins;
    priv->def_auto &= (priv->trig_rise|priv->trig_fall);

    // copy auto-arm defaults to the auto-arm register (the register may be manipulated by commands)
    priv->arm_auto = priv->def_auto;
    priv->arm_single = 0;

    // clear countdowns
    memset(priv->holdoff_countdowns, 0, sizeof(priv->holdoff_countdowns));

    // --- Parse config ---
    priv->port = hw_port2periph(priv->port_name, &suc);
    if (!suc) return E_BAD_CONFIG;

    // Claim all needed pins
    TRY(rsc_claim_gpios(unit, priv->port_name, priv->pins));

    uint16_t mask;

    // claim the needed EXTIs
    mask = 1;
    for (int i = 0; i < 16; i++, mask <<= 1) {
        if (priv->pins & mask) {
            if ((priv->trig_rise|priv->trig_fall) & mask) {
                TRY(rsc_claim(unit, R_EXTI0+i));
            }
        }
    }

    mask = 1;
    for (int i = 0; i < 16; i++, mask <<= 1) {
        if (priv->pins & mask) {
            uint32_t ll_pin = hw_pin2ll((uint8_t) i, &suc);

            // --- Init hardware ---
            LL_GPIO_SetPinMode(priv->port, ll_pin, LL_GPIO_MODE_INPUT);

            uint32_t pull = 0;

#if PLAT_NO_FLOATING_INPUTS
            pull = LL_GPIO_PULL_UP;
#else
            pull = LL_GPIO_PULL_NO;
#endif

            if (priv->pulldown & mask) pull = LL_GPIO_PULL_DOWN;
            if (priv->pullup & mask) pull = LL_GPIO_PULL_UP;
            LL_GPIO_SetPinPull(priv->port, ll_pin, pull);

            if ((priv->trig_rise|priv->trig_fall) & mask) {
                LL_EXTI_EnableIT_0_31(LL_EXTI_LINES[i]);

                if (priv->trig_rise & mask) {
                    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINES[i]);
                }
                if (priv->trig_fall & mask) {
                    LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINES[i]);
                }

                LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTS[priv->port_name-'A'], LL_SYSCFG_EXTI_LINES[i]);

                irqd_attach(EXTIS[i], DIn_handleExti, unit);
            }
        }
    }

    // request ticks if we have triggers and any hold-offs configured
    if ((priv->trig_rise|priv->trig_fall) && priv->trig_holdoff > 0) {
        unit->tick_interval = 1;
    }

    return E_SUCCESS;
}


/** Tear down the unit */
void DIn_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // pins are de-inited during teardown

    // Detach EXTI handlers and disable interrupts
    const uint16_t triggs = priv->trig_rise | priv->trig_fall;
    if (unit->status == E_SUCCESS && triggs) {
        uint16_t mask = 1;
        for (int i = 0; i < 16; i++, mask <<= 1) {
            if (triggs & mask) {
                LL_EXTI_DisableIT_0_31(LL_EXTI_LINES[i]);
                irqd_detach(EXTIS[i], DIn_handleExti);
            }
        }
    }

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
