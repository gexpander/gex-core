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

    priv->cfg.ch1.buffered = true;
    priv->cfg.ch1.enable = true;
    priv->cfg.ch1.noise_level = 2;
    priv->cfg.ch1.noise_type = NOISE_NONE;

    priv->ch1.noise_type = priv->cfg.ch1.noise_type;
    priv->ch1.noise_level = priv->cfg.ch1.noise_level;

    priv->cfg.ch2.buffered = true;
    priv->cfg.ch2.enable = true;
    priv->cfg.ch2.noise_level = 2;
    priv->cfg.ch2.noise_type = NOISE_NONE;

    priv->ch2.noise_type = priv->cfg.ch2.noise_type;
    priv->ch2.noise_level = priv->cfg.ch2.noise_level;

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t UDAC_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    // this may change for different devices
    const Resource r_ch1 = R_PA4;
    const Resource r_ch2 = R_PA5;

    const bool e1 = priv->cfg.ch1.enable;
    const bool e2 = priv->cfg.ch2.enable;


    if (e1) {
        TRY(rsc_claim(unit, r_ch1));
    }

    if (e2) {
        TRY(rsc_claim(unit, r_ch2));
    }

    TRY(rsc_claim(unit, R_DAC1));

    // ensure the peripheral is clean (this may be redundant)
    __HAL_RCC_DAC1_FORCE_RESET();
    __HAL_RCC_DAC1_RELEASE_RESET();

    hw_periph_clock_enable(DAC1);

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

    UDAC_Reconfigure(unit);

    return E_SUCCESS;
}


/** Tear down the unit */
void UDAC_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init peripherals
    if (unit->status == E_SUCCESS ) {
        LL_DAC_DeInit(DAC);
        hw_periph_clock_disable(DAC1);
    }

    // Release all resources, deinit pins
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
