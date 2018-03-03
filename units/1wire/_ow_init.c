//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define OW_INTERNAL
#include "_ow_internal.h"

/** Allocate data structure and set defaults */
error_t OW_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    // some defaults
    priv->pin_number = 0;
    priv->port_name = 'A';
    priv->parasitic = false;

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t OW_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    // --- Parse config ---
    priv->ll_pin = hw_pin2ll(priv->pin_number, &suc);
    priv->port = hw_port2periph(priv->port_name, &suc);
    Resource rsc = rsc_portpin2rsc(priv->port_name, priv->pin_number, &suc);
    if (!suc) return E_BAD_CONFIG;

    // --- Claim resources ---
    TRY(rsc_claim(unit, rsc));

    // --- Init hardware ---
    LL_GPIO_SetPinMode(priv->port, priv->ll_pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(priv->port, priv->ll_pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(priv->port, priv->ll_pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinPull(priv->port, priv->ll_pin, LL_GPIO_PULL_UP); // pull-up for OD state

    return E_SUCCESS;
}

/** Tear down the unit */
void OW_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
