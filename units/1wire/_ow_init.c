//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define OW_INTERNAL
#include "_ow_internal.h"
#include "_ow_init.h"

/** Allocate data structure and set defaults */
error_t OW_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    // the timer is not started until needed
    priv->busyWaitTimer = xTimerCreate("1w_tim", // name
                                       750,      // interval (will be changed when starting it)
                                       true,     // periodic (we use this only for the polling variant, the one-shot will stop the timer in the CB)
                                       unit,     // user data
                                       OW_TimerCb); // callback

    if (priv->busyWaitTimer == NULL) return E_OUT_OF_MEM;

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
    Resource rsc = hw_pin2resource(priv->port_name, priv->pin_number, &suc);
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

    // Delete the software timer
    assert_param(pdPASS == xTimerDelete(priv->busyWaitTimer, 1000));

    // Free memory
    free_ck(unit->data);
}
