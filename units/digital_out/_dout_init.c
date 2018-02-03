//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define DOUT_INTERNAL
#include "_dout_internal.h"

/** Allocate data structure and set defaults */
error_t DOut_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    // some defaults
    priv->port_name = 'A';
    priv->pins = 0x0001;
    priv->open_drain = 0x0000;
    priv->initial = 0x0000;

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t DOut_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    priv->initial &= priv->pins;
    priv->open_drain &= priv->pins;

    // --- Parse config ---
    priv->port = hw_port2periph(priv->port_name, &suc);
    if (!suc) return E_BAD_CONFIG;

    // Claim all needed pins
    TRY(rsc_claim_gpios(unit, priv->port_name, priv->pins));

    for (int i = 0; i < 16; i++) {
        if (priv->pins & (1 << i)) {
            uint32_t ll_pin = hw_pin2ll((uint8_t) i, &suc);

            // --- Init hardware ---
            LL_GPIO_SetPinMode(priv->port, ll_pin, LL_GPIO_MODE_OUTPUT);
            LL_GPIO_SetPinOutputType(priv->port, ll_pin,
                                     (priv->open_drain & (1 << i)) ? LL_GPIO_OUTPUT_OPENDRAIN : LL_GPIO_OUTPUT_PUSHPULL);
            LL_GPIO_SetPinSpeed(priv->port, ll_pin, LL_GPIO_SPEED_FREQ_HIGH);
        }
    }

    // Set the initial state
    priv->port->ODR &= ~priv->pins;
    priv->port->ODR |= priv->initial;

    return E_SUCCESS;
}

/** Tear down the unit */
void DOut_deInit(Unit *unit)
{
    // pins are de-inited during teardown

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
