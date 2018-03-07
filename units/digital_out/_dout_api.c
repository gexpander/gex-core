//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_dout.h"

#define DOUT_INTERNAL

#include "_dout_internal.h"

static void clear_pulse_by_mask(struct priv *priv, uint16_t spread)
{
    assert_param(priv);

    for (int i = 0; i < 16; i++) {
        if (spread & (1 << i)) {
            priv->msec_pulse_cnt[i] = 0;
        }
    }
}

error_t UU_DOut_Write(Unit *unit, uint16_t packed)
{
    CHECK_TYPE(unit, &UNIT_DOUT);

    struct priv *priv = unit->data;
    uint16_t mask = priv->pins;
    uint16_t spread = pinmask_spread(packed, mask);
    clear_pulse_by_mask(priv, spread);

    uint16_t set = spread;
    uint16_t reset = ((~spread) & mask);
    priv->port->BSRR = set | (reset << 16);
    return E_SUCCESS;
}

error_t UU_DOut_Set(Unit *unit, uint16_t packed)
{
    CHECK_TYPE(unit, &UNIT_DOUT);

    struct priv *priv = unit->data;
    uint16_t mask = priv->pins;
    uint16_t spread = pinmask_spread(packed, mask);
    clear_pulse_by_mask(priv, spread);

    priv->port->BSRR = spread;
    return E_SUCCESS;
}

error_t UU_DOut_Clear(Unit *unit, uint16_t packed)
{
    CHECK_TYPE(unit, &UNIT_DOUT);

    struct priv *priv = unit->data;
    uint16_t mask = priv->pins;
    uint16_t spread = pinmask_spread(packed, mask);
    clear_pulse_by_mask(priv, spread);

    priv->port->BSRR = (spread << 16);
    return E_SUCCESS;
}

error_t UU_DOut_Toggle(Unit *unit, uint16_t packed)
{
    CHECK_TYPE(unit, &UNIT_DOUT);

    struct priv *priv = unit->data;
    uint16_t mask = priv->pins;
    uint16_t spread = pinmask_spread(packed, mask);
    clear_pulse_by_mask(priv, spread);

    uint16_t flipped = (uint16_t) (~priv->port->ODR) & mask;
    uint16_t set = flipped & spread;
    uint16_t reset = ((~flipped) & mask) & spread;
    priv->port->BSRR = set | (reset << 16);
    return E_SUCCESS;
}

error_t UU_DOut_GetPinCount(Unit *unit, uint8_t *count)
{
    CHECK_TYPE(unit, &UNIT_DOUT);
    struct priv *priv = unit->data;

    uint32_t packed = pinmask_pack(0xFFFF, priv->pins);
    *count = (uint8_t) (32 - __CLZ(packed));
    return E_SUCCESS;
}

error_t UU_DOut_Pulse(Unit *unit, uint16_t packed, bool polarity, bool is_usec, uint16_t count)
{
    CHECK_TYPE(unit, &UNIT_DOUT);
    struct priv *priv = unit->data;
    assert_param(priv);

    uint16_t mask = priv->pins;
    uint16_t spread = pinmask_spread(packed, mask);
    clear_pulse_by_mask(priv, spread);

    vPortEnterCritical();

    if (is_usec) {
        // we're gonna do this right here as a delay loop.
        if (count >= 1000) {
            // too long, fall back to msec
            count /= 1000;
            is_usec = false;
        }
        else {
            const uint32_t bsrr1 = spread << (polarity ? 0 : 16);
            const uint32_t bsrr0 = spread << (polarity ? 16 : 0);

            const uint32_t start = PTIM_MicroDelayAlign();
            priv->port->BSRR = bsrr1;
            PTIM_MicroDelayAligned(count, start);
            priv->port->BSRR = bsrr0;
        }
    }

    if (!is_usec) {
        // Load the counters
        for (int i = 0; i < 16; i++) {
            if (spread & (1 << i)) {
                priv->msec_pulse_cnt[i] = (uint16_t) (count + 1);
            }
        }

        if (polarity) {
            priv->msec_pulse_scheduled_1 |= spread;
        } else {
            priv->msec_pulse_scheduled_0 |= spread;
        }

        unit->_tick_cnt = 0;
        unit->tick_interval = 1;
    }

    vPortExitCritical();

    return E_SUCCESS;
}

void DOut_Tick(Unit *unit)
{
    struct priv *priv = unit->data;

    uint16_t odr = (uint16_t) priv->port->ODR;
    int live_cnt = 0;
    for (int i = 0; i < 16; i++) {
        if (priv->msec_pulse_scheduled_1 & (1<<i)) {
            odr |= (1<<i);
        } else if (priv->msec_pulse_scheduled_0 & (1<<i)) {
            odr &= ~(1<<i);
        }

        if (priv->msec_pulse_cnt[i] > 0) {
            live_cnt++;
            priv->msec_pulse_cnt[i]--;
            if (priv->msec_pulse_cnt[i] == 0) {
                odr ^= 1 << i;
            }
        }
    }
    priv->port->ODR = odr;
    priv->msec_pulse_scheduled_1 = 0;
    priv->msec_pulse_scheduled_0 = 0;

    if (live_cnt == 0) {
        unit->_tick_cnt = 0;
        unit->tick_interval = 0;
    }
}
