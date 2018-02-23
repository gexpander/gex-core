//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define SIPO_INTERNAL
#include "_sipo_internal.h"

/** Allocate data structure and set defaults */
error_t USIPO_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    priv->cfg.pin_store = R_PA0;
    priv->cfg.store_pol = true;

    priv->cfg.pin_shift = R_PA1;
    priv->cfg.shift_pol = true;

    priv->cfg.pin_clear = R_PA2;
    priv->cfg.clear_pol = false;

    priv->cfg.data_pname = 'A';
    priv->cfg.data_pins = (1<<3);

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t USIPO_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    // --- Parse config ---
    suc &= hw_pinrsc2ll(priv->cfg.pin_store, &priv->store_port, &priv->store_ll);
    suc &= hw_pinrsc2ll(priv->cfg.pin_shift, &priv->shift_port, &priv->shift_ll);
    suc &= hw_pinrsc2ll(priv->cfg.pin_clear, &priv->clear_port, &priv->clear_ll);
    if (!suc) return E_BAD_CONFIG;
    TRY(rsc_claim(unit, priv->cfg.pin_store));
    TRY(rsc_claim(unit, priv->cfg.pin_shift));
    TRY(rsc_claim(unit, priv->cfg.pin_clear));

    // Claim all needed pins
    TRY(rsc_claim_gpios(unit, priv->cfg.data_pname, priv->cfg.data_pins));
    priv->data_port = hw_port2periph(priv->cfg.data_pname, &suc);

    // --- Init hardware ---

    priv->data_width = 0;
    for (int i = 0; i < 16; i++) {
        if (priv->cfg.data_pins & (1 << i)) {
            uint32_t ll_pin = hw_pin2ll((uint8_t) i, &suc);
            LL_GPIO_SetPinMode(priv->data_port, ll_pin, LL_GPIO_MODE_OUTPUT);
            LL_GPIO_SetPinOutputType(priv->data_port, ll_pin, LL_GPIO_OUTPUT_PUSHPULL);
            LL_GPIO_SetPinSpeed(priv->data_port, ll_pin, LL_GPIO_SPEED_FREQ_HIGH);
            priv->data_width++;
        }
    }

    // Set the initial state - zeros
    priv->data_port->ODR &= ~priv->cfg.data_pins;


    // STORE
    LL_GPIO_SetPinMode(priv->store_port, priv->store_ll, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(priv->store_port, priv->store_ll, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(priv->store_port, priv->store_ll, LL_GPIO_SPEED_FREQ_HIGH);

    if (priv->cfg.store_pol)
        LL_GPIO_ResetOutputPin(priv->store_port, priv->store_ll);
    else
        LL_GPIO_SetOutputPin(priv->store_port, priv->store_ll);

    // SHIFT
    LL_GPIO_SetPinMode(priv->shift_port, priv->shift_ll, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(priv->shift_port, priv->shift_ll, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(priv->shift_port, priv->shift_ll, LL_GPIO_SPEED_FREQ_HIGH);

    if (priv->cfg.shift_pol)
        LL_GPIO_ResetOutputPin(priv->shift_port, priv->shift_ll);
    else
        LL_GPIO_SetOutputPin(priv->shift_port, priv->shift_ll);

    // CLEAR
    LL_GPIO_SetPinMode(priv->clear_port, priv->clear_ll, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(priv->clear_port, priv->clear_ll, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(priv->clear_port, priv->clear_ll, LL_GPIO_SPEED_FREQ_HIGH);

    if (priv->cfg.clear_pol)
        LL_GPIO_ResetOutputPin(priv->clear_port, priv->clear_ll);
    else
        LL_GPIO_SetOutputPin(priv->clear_port, priv->clear_ll);

    // initial clear
    UU_SIPO_DirectClear(unit);

    return E_SUCCESS;
}


/** Tear down the unit */
void USIPO_deInit(Unit *unit)
{
    // Release all resources, deinit pins
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
