//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define NPX_INTERNAL
#include "_npx_internal.h"
#include "_npx_init.h"
#include "ws2812.h"

/** Allocate data structure and set defaults */
error_t Npx_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL)

        // some defaults
        priv->pin_number = 0;
    priv->port_name = 'A';
    priv->pixels = 1;

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t Npx_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    // --- Parse config ---
    priv->ll_pin = hw_pin2ll(priv->pin_number, &suc);
    priv->port = hw_port2periph(priv->port_name, &suc);
    Resource rsc = hw_pin2resource(priv->port_name, priv->pin_number, &suc);
    if (!suc) return E_BAD_CONFIG;

    // --- Claim resources ---
    TRY(rsc_claim(unit, rsc));

    // --- Init hardware ---
    LL_GPIO_SetPinMode(priv->port, priv->ll_pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(priv->port, priv->ll_pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(priv->port, priv->ll_pin, LL_GPIO_SPEED_FREQ_HIGH);

    // clear strip

    ws2812_clear(priv->port, priv->ll_pin, priv->pixels);

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
