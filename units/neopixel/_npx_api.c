//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_neopixel.h"

#define NPX_INTERNAL
#include "_npx_internal.h"
#include "ws2812.h"

/* Clear the strip */
error_t UU_Npx_Clear(Unit *unit)
{
    CHECK_TYPE(unit, &UNIT_NEOPIXEL);

    struct priv *priv = unit->data;
    ws2812_clear(priv->port, priv->ll_pin, priv->cfg.pixels);
    return E_SUCCESS;
}

/* Load packed */
error_t UU_Npx_Load(Unit *unit, const uint8_t *packed_rgb, uint32_t nbytes)
{
    CHECK_TYPE(unit, &UNIT_NEOPIXEL);

    struct priv *priv = unit->data;
    if (nbytes != 3*priv->cfg.pixels) return E_BAD_COUNT;
    ws2812_load_raw(priv->port, priv->ll_pin, packed_rgb, priv->cfg.pixels);
    return E_SUCCESS;
}

/* Load U32, LE or BE */
error_t UU_Npx_Load32(Unit *unit, const uint8_t *bytes, uint32_t nbytes, bool order_bgr, bool zero_before)
{
    CHECK_TYPE(unit, &UNIT_NEOPIXEL);

    struct priv *priv = unit->data;
    if (nbytes != 4*priv->cfg.pixels) return E_BAD_COUNT;
    ws2812_load_sparse(priv->port, priv->ll_pin, bytes, priv->cfg.pixels, order_bgr, zero_before);
    return E_SUCCESS;
}

/* Get the pixel count */
error_t UU_Npx_GetCount(Unit *unit, uint16_t *count)
{
    CHECK_TYPE(unit, &UNIT_NEOPIXEL);

    struct priv *priv = unit->data;
    *count = priv->cfg.pixels;
    return E_SUCCESS;
}
