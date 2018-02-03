//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_din.h"

#define DIN_INTERNAL

#include "_din_internal.h"

/** Read request */
error_t UU_DIn_Read(Unit *unit, uint16_t *packed)
{
    CHECK_TYPE(unit, &UNIT_DIN);
    struct priv *priv = unit->data;
    *packed = pinmask_pack((uint16_t) priv->port->IDR, priv->pins);
    return E_SUCCESS;
}

/** Arm pins */
error_t UU_DIn_Arm(Unit *unit, uint16_t arm_single_packed, uint16_t arm_auto_packed)
{
    CHECK_TYPE(unit, &UNIT_DIN);
    struct priv *priv = unit->data;

    uint16_t arm_single = pinmask_spread(arm_single_packed, priv->pins);
    uint16_t arm_auto = pinmask_spread(arm_auto_packed, priv->pins);

    // abort if user tries to arm pin that doesn't have a trigger configured
    if (0 != ((arm_single | arm_auto) & ~(priv->trig_fall | priv->trig_rise))) {
        return E_BAD_VALUE;
    }

    // arm and reset hold-offs
    // we use critical section to avoid irq between the two steps
    vPortEnterCritical();
    {
        priv->arm_auto |= arm_single;
        priv->arm_single |= arm_auto;
        const uint16_t combined = arm_single | arm_auto;
        for (int i = 0; i < 16; i++) {
            if (combined & (1 << i)) {
                priv->holdoff_countdowns[i] = 0;
            }
        }
    }
    vPortExitCritical();

    return E_SUCCESS;
}

/** DisArm pins */
error_t UU_DIn_DisArm(Unit *unit, uint16_t disarm_packed)
{
    CHECK_TYPE(unit, &UNIT_DIN);
    struct priv *priv = unit->data;

    uint16_t disarm = pinmask_spread(disarm_packed, priv->pins);

    // abort if user tries to disarm pin that doesn't have a trigger configured
    if (0 != ((disarm) & ~(priv->trig_fall | priv->trig_rise))) {
        return E_BAD_VALUE;
    }

    priv->arm_auto &= ~disarm;
    priv->arm_single &= ~disarm;

    return E_SUCCESS;
}
