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

    priv->signal_pname = 'A';
    priv->signal_pnum = 0;

    priv->opmode = OPMODE_PWM_CONT;

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

    uint32_t ll_ch_a = 0;
    uint32_t ll_ch_b = 0;

    switch (priv->signal_pname) {
        case 'A':
            switch (priv->signal_pnum) {
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
            switch (priv->signal_pnum) {
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

    TRY(rsc_claim_pin(unit, priv->signal_pname, priv->signal_pnum));
    TRY(rsc_claim(unit, timRsc));

    // ---- INIT ----
    assert_param(ll_ch_a != ll_ch_b);

    priv->TIMx = TIMx;
    priv->ll_ch_a = ll_ch_a;
    priv->ll_ch_b = ll_ch_b;
    priv->a_direct = a_direct;

    TRY(hw_configure_gpio_af(priv->signal_pname, priv->signal_pnum, ll_timpin_af));

    hw_periph_clock_enable(TIMx);
    irqd_attach(TIMx, UFCAP_TimerHandler, unit);

    UFCAP_SwitchMode(unit, OPMODE_IDLE);

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
        LL_TIM_DeInit(TIMx);
        irqd_attach(TIMx, UFCAP_TimerHandler, unit);
    }

    // Release all resources, deinit pins
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
