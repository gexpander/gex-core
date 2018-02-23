//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_sipo.h"

#define SIPO_INTERNAL
#include "_sipo_internal.h"

static void send_pulse(bool pol, GPIO_TypeDef *port, uint32_t ll)
{
    if (pol) {
        LL_GPIO_SetOutputPin(port, ll);
    }
    else {
        LL_GPIO_ResetOutputPin(port, ll);
    }

    __asm_loop(2);

    if (pol) {
        LL_GPIO_ResetOutputPin(port, ll);
    }
    else {
        LL_GPIO_SetOutputPin(port, ll);
    }
}

#pragma GCC push_options
#pragma GCC optimize ("O2")
error_t UU_SIPO_Write(Unit *unit, const uint8_t *buffer, uint16_t buflen, uint16_t terminal_data)
{
    CHECK_TYPE(unit, &UNIT_SIPO);
    struct priv *priv = unit->data;

    if (buflen % priv->data_width != 0) {
        dbg("Buflen %d vs width %d", (int)buflen, (int)priv->data_width);
        return E_BAD_COUNT; // must be a multiple of the channel count
    }

    // buffer contains data for the individual data pins, back to back as AAA BBB CCC (whole bytes)
    const uint8_t data_width = priv->data_width;
    const uint16_t bytelen = buflen / data_width;
    const uint16_t mask = priv->cfg.data_pins;

    uint8_t offsets[16];
    for (int i=0; i<16; i++) offsets[i] = (uint8_t) (bytelen * i);

    for (int32_t bn = bytelen - 1; bn >= 0; bn--) {
        // send the byte
        for (int32_t i = 0; i < 8; i++) {
            uint16_t packed = 0;
            for (int32_t j = data_width - 1; j >= 0; j--) {
                packed |= (buffer[bn + offsets[j]] >> i) & 1;
                if (j > 0) packed <<= 1;
            }

            uint16_t spread = pinmask_spread(packed, mask);
            priv->data_port->BSRR = spread | (((~spread) & mask) << 16);

            // Shift clock pulse
            send_pulse(priv->cfg.shift_pol, priv->shift_port, priv->shift_ll);
        }
    }

    // load the final data - this may be used by some other circuitry or
    // simply to rest the lines at a defined known level
    uint16_t spread = pinmask_spread(terminal_data, mask);
    priv->data_port->BSRR = spread | (((~spread) & mask) << 16);

    send_pulse(priv->cfg.store_pol, priv->store_port, priv->store_ll);
    return E_SUCCESS;
}
#pragma GCC pop_options

error_t UU_SIPO_DirectData(Unit *unit, uint16_t data_packed)
{
    CHECK_TYPE(unit, &UNIT_SIPO);
    struct priv *priv = unit->data;

    uint16_t spread = pinmask_spread(data_packed, priv->cfg.data_pins);
    priv->data_port->BSRR = spread | (((~spread) & priv->cfg.data_pins) << 16);
    return E_SUCCESS;
}

error_t UU_SIPO_DirectClear(Unit *unit)
{
    CHECK_TYPE(unit, &UNIT_SIPO);
    struct priv *priv = unit->data;
    send_pulse(priv->cfg.clear_pol, priv->clear_port, priv->clear_ll);
    return E_SUCCESS;
}

error_t UU_SIPO_DirectShift(Unit *unit)
{
    CHECK_TYPE(unit, &UNIT_SIPO);
    struct priv *priv = unit->data;
    send_pulse(priv->cfg.shift_pol, priv->shift_port, priv->shift_ll);
    return E_SUCCESS;
}

error_t UU_SIPO_DirectStore(Unit *unit)
{
    CHECK_TYPE(unit, &UNIT_SIPO);
    struct priv *priv = unit->data;
    send_pulse(priv->cfg.store_pol, priv->store_port, priv->store_ll);
    return E_SUCCESS;
}
