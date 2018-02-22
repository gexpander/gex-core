//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define FCAP_INTERNAL
#include "_fcap_internal.h"

/** Allocate data structure and set defaults */
error_t UFCAP_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    priv->conf.signal_pname = 'A';
    priv->conf.signal_pnum = 0;

    priv->conf.active_level = 1;
    priv->conf.direct_presc = 1;
    priv->conf.dfilter = 0;
    priv->conf.direct_msec = 1000;
    priv->conf.startmode = OPMODE_IDLE;

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t UFCAP_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    // ---- Resolve what to configure ----

    TIM_TypeDef * const TIMx = TIM2;
    Resource timRsc = R_TIM2;

    TIM_TypeDef * const TIMy = TIM14;
    Resource tim2Rsc = R_TIM14;

    uint32_t ll_ch_a = 0;
    uint32_t ll_ch_b = 0;

    switch (priv->conf.signal_pname) {
        case 'A':
            switch (priv->conf.signal_pnum) {
                case 5:
                case 15:
                case 0: ll_ch_a = LL_TIM_CHANNEL_CH1; break;
                case 1: ll_ch_a = LL_TIM_CHANNEL_CH2; break;
                default:
                    dbg("Bad signal pin!");
                    return E_BAD_CONFIG;
            }
            break;
        case 'B':
            switch (priv->conf.signal_pnum) {
                case 3: ll_ch_a = LL_TIM_CHANNEL_CH2; break;
                default:
                    dbg("Bad signal pin!");
                    return E_BAD_CONFIG;
            }
            break;

        default:
            dbg("Bad signal pin port!");
            return E_BAD_CONFIG;
    }
    const uint32_t ll_timpin_af = LL_GPIO_AF_2;

    bool a_direct = true;
    switch (ll_ch_a) {
        case LL_TIM_CHANNEL_CH1:
            ll_ch_b = LL_TIM_CHANNEL_CH2;
            break;

        case LL_TIM_CHANNEL_CH2:
            ll_ch_b = LL_TIM_CHANNEL_CH1;
            a_direct = false;
            break;
    }

    // ---- CLAIM ----

    TRY(rsc_claim_pin(unit, priv->conf.signal_pname, priv->conf.signal_pnum));
    TRY(rsc_claim(unit, timRsc));
    TRY(rsc_claim(unit, tim2Rsc));

    // ---- INIT ----
    assert_param(ll_ch_a != ll_ch_b);

    priv->TIMx = TIMx;
    priv->TIMy = TIMy;
    priv->ll_ch_a = ll_ch_a;
    priv->ll_ch_b = ll_ch_b;
    priv->a_direct = a_direct;

    // Load defaults
    priv->active_level = priv->conf.active_level;
    priv->direct_presc = priv->conf.direct_presc;
    priv->dfilter = priv->conf.dfilter;
    priv->direct_msec = priv->conf.direct_msec;
    priv->opmode = priv->conf.startmode;

    TRY(hw_configure_gpio_af(priv->conf.signal_pname, priv->conf.signal_pnum, ll_timpin_af));

    GPIO_TypeDef *gpio = hw_port2periph(priv->conf.signal_pname, &suc);
    uint32_t ll_pin = hw_pin2ll(priv->conf.signal_pnum, &suc);
    LL_GPIO_SetPinPull(gpio, ll_pin, LL_GPIO_PULL_DOWN); // XXX change to pull-up if the polarity is inverted

    hw_periph_clock_enable(TIMx);
    hw_periph_clock_enable(TIMy);
    irqd_attach(TIMx, UFCAP_TIMxHandler, unit);
    irqd_attach(TIMy, UFCAP_TIMyHandler, unit);

    UFCAP_SwitchMode(unit, priv->opmode); // switch to the default opmode

    return E_SUCCESS;
}

/** Tear down the unit */
void UFCAP_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init peripherals
    if (unit->status == E_SUCCESS ) {
        UFCAP_SwitchMode(unit, OPMODE_IDLE);

        TIM_TypeDef *TIMx = priv->TIMx;
        TIM_TypeDef *TIMy = priv->TIMy;
        LL_TIM_DeInit(TIMx);
        LL_TIM_DeInit(TIMy);
        irqd_detach(TIMx, UFCAP_TIMxHandler);
        irqd_detach(TIMy, UFCAP_TIMyHandler);
        hw_periph_clock_disable(TIMx);
        hw_periph_clock_disable(TIMy);
    }

    // Release all resources, deinit pins
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
