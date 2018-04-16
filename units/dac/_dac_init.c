//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define DAC_INTERNAL
#include "_dac_internal.h"

/** Allocate data structure and set defaults */
error_t UDAC_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    for (int i = 0; i < 2; i++) {
        priv->cfg.ch[i].buffered = true;
        priv->cfg.ch[i].enable = true;
        priv->cfg.ch[i].noise_level = 2;
        priv->cfg.ch[i].noise_type = NOISE_NONE;

        priv->ch[i].waveform = UDAC_WAVE_DC;
        priv->ch[i].dc_level = 2047;

        priv->ch[i].rectangle_ontime = 4096; // half
        priv->ch[i].rectangle_high = 4095;
        priv->ch[i].rectangle_low = 0;

        priv->ch[i].counter = 0;
        priv->ch[i].increment = 0; // stopped
        priv->ch[i].phase = 0;
    }

    UDAC_SetFreq(unit, 0, 1000);
    UDAC_SetFreq(unit, 1, 1000);

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t UDAC_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    // copy noise config
    priv->ch[0].noise_type = priv->cfg.ch[0].noise_type;
    priv->ch[0].noise_level = priv->cfg.ch[0].noise_level;

    priv->ch[1].noise_type = priv->cfg.ch[1].noise_type;
    priv->ch[1].noise_level = priv->cfg.ch[1].noise_level;

    // this may change for different devices
    const Resource r_ch1 = R_PA4;
    const Resource r_ch2 = R_PA5;

    TRY(rsc_claim(unit, R_TIM6));
    priv->TIMx = TIM6;

    const bool e1 = priv->cfg.ch[0].enable;
    const bool e2 = priv->cfg.ch[1].enable;

    if (e1) {
        TRY(rsc_claim(unit, r_ch1));
    }

    if (e2) {
        TRY(rsc_claim(unit, r_ch2));
    }

    TRY(rsc_claim(unit, R_DAC1));

    hw_periph_clock_enable(DAC1);
    hw_periph_clock_enable(priv->TIMx);

    GPIO_TypeDef *port;
    uint32_t ll;
    if (e1) {
        assert_param(hw_pinrsc2ll(r_ch1, &port, &ll));
        LL_GPIO_SetPinMode(port, ll, LL_GPIO_MODE_ANALOG);
    }
    if (e2) {
        assert_param(hw_pinrsc2ll(r_ch1, &port, &ll));
        LL_GPIO_SetPinMode(port, ll, LL_GPIO_MODE_ANALOG);
    }

    uint16_t presc = 1;

    // presets... TODO pick the highest useable one (or find a new one)
#if UDAC_TIM_FREQ_DIVIDER == 1
    uint32_t count = PLAT_AHB_MHZ;
#elif UDAC_TIM_FREQ_DIVIDER == 2
    uint32_t count = PLAT_AHB_MHZ * 2;
#elif UDAC_TIM_FREQ_DIVIDER == 4
    uint32_t count = PLAT_AHB_MHZ * 4;
#elif UDAC_TIM_FREQ_DIVIDER == 8
    uint32_t count = PLAT_AHB_MHZ * 8;
#else
    #error "bad freq"
#endif

//    dbg("Presc %d, count %d", (int)presc, (int)count);

    LL_TIM_SetPrescaler(priv->TIMx, (uint32_t) (presc - 1));
    LL_TIM_SetAutoReload(priv->TIMx, count - 1);
    LL_TIM_EnableARRPreload(priv->TIMx);
    LL_TIM_GenerateEvent_UPDATE(priv->TIMx);
    LL_TIM_ClearFlag_UPDATE(priv->TIMx); // prevent irq right after enabling

    irqd_attach(priv->TIMx, UDAC_HandleIT, unit);
    LL_TIM_EnableIT_UPDATE(priv->TIMx);

    UDAC_Reconfigure(unit); // works with the timer - it should be inited already

    // do not enbale counter initially - no need
//    LL_TIM_EnableCounter(priv->TIMx);

    return E_SUCCESS;
}


/** Tear down the unit */
void UDAC_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init peripherals
    if (unit->status == E_SUCCESS ) {
        LL_DAC_DeInit(DAC);
        LL_TIM_DeInit(priv->TIMx);

        hw_periph_clock_disable(DAC1);
        hw_periph_clock_disable(priv->TIMx);

        irqd_detach(priv->TIMx, UDAC_HandleIT);
    }

    // Release all resources, deinit pins
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
