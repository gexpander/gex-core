//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_dout.h"

#define DOUT_INTERNAL
#include "_dout_internal.h"

error_t UU_DOut_Write(Unit *unit, uint16_t packed)
{
    CHECK_TYPE(unit, &UNIT_DOUT);

    struct priv *priv = unit->data;
    uint16_t mask = priv->pins;
    uint16_t spread = pinmask_spread(packed, mask);

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

    priv->port->BSRR = spread;
    return E_SUCCESS;
}

error_t UU_DOut_Clear(Unit *unit, uint16_t packed)
{
    CHECK_TYPE(unit, &UNIT_DOUT);

    struct priv *priv = unit->data;
    uint16_t mask = priv->pins;
    uint16_t spread = pinmask_spread(packed, mask);

    priv->port->BSRR = (spread<<16);
    return E_SUCCESS;
}

error_t UU_DOut_Toggle(Unit *unit, uint16_t packed)
{
    CHECK_TYPE(unit, &UNIT_DOUT);

    struct priv *priv = unit->data;
    uint16_t mask = priv->pins;
    uint16_t spread = pinmask_spread(packed, mask);

    uint16_t flipped = (uint16_t) (~priv->port->ODR) & mask;
    uint16_t set = flipped & spread;
    uint16_t reset = ((~flipped) & mask) & spread;
    priv->port->BSRR = set | (reset<<16);
    return E_SUCCESS;
}

error_t UU_DOut_GetPinCount(Unit *unit, uint8_t *count)
{
    CHECK_TYPE(unit, &UNIT_DOUT);
    struct priv *priv = unit->data;

    uint32_t packed = pinmask_pack(0xFFFF, priv->pins);
    *count = (uint8_t)(32 - __CLZ(packed));
    return E_SUCCESS;
}
