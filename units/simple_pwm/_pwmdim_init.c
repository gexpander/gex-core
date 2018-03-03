//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define PWMDIM_INTERNAL
#include "_pwmdim_internal.h"

/** Allocate data structure and set defaults */
error_t UPWMDIM_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    priv->cfg.freq = 1000;
    priv->cfg.ch1_choice = 1;
    priv->cfg.ch2_choice = 0;
    priv->cfg.ch3_choice = 0;
    priv->cfg.ch4_choice = 0;

    priv->duty1 = 500;
    priv->duty2 = 500;
    priv->duty3 = 500;
    priv->duty4 = 500;

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t UPWMDIM_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    TRY(rsc_claim(unit, R_TIM3));
    priv->TIMx = TIM3;
    hw_periph_clock_enable(priv->TIMx);

    // copy the default frequency
    priv->freq = priv->cfg.freq;

    const Resource ch1_pins[] = { R_PA6, R_PB4, R_PC6 };
    const uint32_t ch1_af[]   = { LL_GPIO_AF_1, LL_GPIO_AF_1, LL_GPIO_AF_0 };

    const Resource ch2_pins[] = { R_PA7, R_PB5, R_PC7 };
    const uint32_t ch2_af[]   = { LL_GPIO_AF_1, LL_GPIO_AF_1, LL_GPIO_AF_0 };

    const Resource ch3_pins[] = { R_PB0, R_PC8 };
    const uint32_t ch3_af[]   = { LL_GPIO_AF_1, LL_GPIO_AF_0 };

    const Resource ch4_pins[] = { R_PB1, R_PC9 };
    const uint32_t ch4_af[]   = { LL_GPIO_AF_1, LL_GPIO_AF_0 };

    Resource r[4] = {};
    uint32_t af[4] = {};

    // --- resolve pins and AFs ---
    if (priv->cfg.ch1_choice > 0) {
        if (priv->cfg.ch1_choice > 3) return E_BAD_CONFIG;
        r[0] = ch1_pins[priv->cfg.ch1_choice - 1];
        af[0] = ch1_af[priv->cfg.ch1_choice - 1];
        TRY(rsc_claim(unit, r[0]));
    }

    if (priv->cfg.ch2_choice > 0) {
        if (priv->cfg.ch2_choice > 3) return E_BAD_CONFIG;
        r[1] = ch2_pins[priv->cfg.ch2_choice - 1];
        af[1] = ch2_af[priv->cfg.ch2_choice - 1];
        TRY(rsc_claim(unit, r[1]));
    }

    if (priv->cfg.ch3_choice > 0) {
        if (priv->cfg.ch3_choice > 2) return E_BAD_CONFIG;
        r[2] = ch3_pins[priv->cfg.ch3_choice - 1];
        af[2] = ch3_af[priv->cfg.ch3_choice - 1];
        TRY(rsc_claim(unit, r[2]));
    }

    if (priv->cfg.ch4_choice > 0) {
        if (priv->cfg.ch4_choice > 2) return E_BAD_CONFIG;
        r[3] = ch4_pins[priv->cfg.ch4_choice - 1];
        af[3] = ch4_af[priv->cfg.ch4_choice - 1];
        TRY(rsc_claim(unit, r[3]));
    }

    // --- configure AF + timer ---
    LL_TIM_DeInit(priv->TIMx); // force a reset

    uint16_t presc;
    uint32_t count;
    float real_freq;
    if (!hw_solve_timer(PLAT_APB1_HZ, priv->freq, true, &presc, &count, &real_freq)) {
        dbg("Failed to resolve timer params.");
        return E_BAD_VALUE;
    }
    LL_TIM_SetPrescaler(priv->TIMx, (uint32_t) (presc - 1));
    LL_TIM_SetAutoReload(priv->TIMx, count - 1);
    LL_TIM_EnableARRPreload(priv->TIMx);

    dbg("Presc %d, cnt %d", (int)presc, (int)count);

    // TODO this can probably be turned into a loop over an array of structs


    if (priv->cfg.ch1_choice > 0) {
        TRY(hw_configure_gpiorsc_af(r[0], af[0]));

        LL_TIM_OC_EnablePreload(priv->TIMx, LL_TIM_CHANNEL_CH1);
        LL_TIM_OC_SetMode(priv->TIMx, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
        LL_TIM_OC_SetCompareCH1(priv->TIMx, count/2);
        LL_TIM_CC_EnablePreload(priv->TIMx);
        LL_TIM_CC_EnableChannel(priv->TIMx, LL_TIM_CHANNEL_CH1);
    }

    if (priv->cfg.ch2_choice > 0) {
        TRY(hw_configure_gpiorsc_af(r[1], af[1]));

        LL_TIM_OC_EnablePreload(priv->TIMx, LL_TIM_CHANNEL_CH2);
        LL_TIM_OC_SetMode(priv->TIMx, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_PWM1);
        LL_TIM_OC_SetCompareCH2(priv->TIMx, count/2);
        LL_TIM_CC_EnableChannel(priv->TIMx, LL_TIM_CHANNEL_CH2);
    }

    if (priv->cfg.ch3_choice > 0) {
        TRY(hw_configure_gpiorsc_af(r[2], af[2]));

        LL_TIM_OC_EnablePreload(priv->TIMx, LL_TIM_CHANNEL_CH3);
        LL_TIM_OC_SetMode(priv->TIMx, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
        LL_TIM_OC_SetCompareCH3(priv->TIMx, count/2);
        LL_TIM_CC_EnableChannel(priv->TIMx, LL_TIM_CHANNEL_CH3);
    }

    if (priv->cfg.ch4_choice > 0) {
        TRY(hw_configure_gpiorsc_af(r[3], af[3]));

        LL_TIM_OC_EnablePreload(priv->TIMx, LL_TIM_CHANNEL_CH4);
        LL_TIM_OC_SetMode(priv->TIMx, LL_TIM_CHANNEL_CH4, LL_TIM_OCMODE_PWM1);
        LL_TIM_OC_SetCompareCH4(priv->TIMx, count/2);
        LL_TIM_CC_EnableChannel(priv->TIMx, LL_TIM_CHANNEL_CH4);
    }

    LL_TIM_GenerateEvent_UPDATE(priv->TIMx);
    LL_TIM_EnableAllOutputs(priv->TIMx);

    // postpone this for later - when user uses the start command.
    // prevents beeping right after restart if used for audio.
//    LL_TIM_EnableCounter(priv->TIMx);

    return E_SUCCESS;
}


/** Tear down the unit */
void UPWMDIM_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init peripherals
    if (unit->status == E_SUCCESS ) {
        LL_TIM_DeInit(priv->TIMx);
    }

    // Release all resources, deinit pins
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
