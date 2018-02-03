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
    ws2812_clear(priv->port, priv->ll_pin, priv->pixels);
    return E_SUCCESS;
}

/* Load packed */
error_t UU_Npx_Load(Unit *unit, const uint8_t *packed_rgb, uint32_t nbytes)
{
    CHECK_TYPE(unit, &UNIT_NEOPIXEL);

    struct priv *priv = unit->data;
    if (nbytes != 3*priv->pixels) return E_BAD_COUNT;
    ws2812_load_raw(priv->port, priv->ll_pin, packed_rgb, priv->pixels);
    return E_SUCCESS;
}

/* Load U32, LE or BE */
static error_t load_u32(Unit *unit, const uint8_t *bytes, uint32_t nbytes, bool bige)
{
    CHECK_TYPE(unit, &UNIT_NEOPIXEL);

    struct priv *priv = unit->data;
    if (nbytes != 4*priv->pixels) return E_BAD_COUNT;
    ws2812_load_sparse(priv->port, priv->ll_pin, bytes, priv->pixels, bige);
    return E_SUCCESS;
}

/* Load U32, LE */
inline error_t UU_Npx_LoadU32LE(Unit *unit, const uint8_t *bytes, uint32_t nbytes)
{
    return load_u32(unit, bytes, nbytes, false);
}

/* Load U32, BE */
inline error_t UU_Npx_LoadU32BE(Unit *unit, const uint8_t *bytes, uint32_t nbytes)
{
    return load_u32(unit, bytes, nbytes, true);
}

/* Get the pixel count */
error_t UU_Npx_GetCount(Unit *unit, uint16_t *count)
{
    CHECK_TYPE(unit, &UNIT_NEOPIXEL);

    struct priv *priv = unit->data;
    *count = priv->pixels;
    return E_SUCCESS;
}
