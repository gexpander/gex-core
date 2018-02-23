//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define NPX_INTERNAL
#include "_npx_internal.h"
#include "ws2812.h"

/** Allocate data structure and set defaults */
error_t Npx_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    // some defaults
    priv->cfg.pin = R_PA0;
    priv->cfg.pixels = 1;

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t Npx_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    // --- Parse config ---
    suc = hw_pinrsc2ll(priv->cfg.pin, &priv->port, &priv->ll_pin);
    if (!suc) return E_BAD_CONFIG;
    TRY(rsc_claim(unit, priv->cfg.pin));

    // --- Init hardware ---
    LL_GPIO_SetPinMode(priv->port, priv->ll_pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(priv->port, priv->ll_pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(priv->port, priv->ll_pin, LL_GPIO_SPEED_FREQ_HIGH);

    // clear strip

    ws2812_clear(priv->port, priv->ll_pin, priv->cfg.pixels);

    return E_SUCCESS;
}

/** Tear down the unit */
void Npx_deInit(Unit *unit)
{
    // pins are de-inited during teardown

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
