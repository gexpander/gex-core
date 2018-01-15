//
// Created by MightyPork on 2017/11/25.
//

#include "utils/avrlibc.h"
#include "comm/messages.h"
#include "unit_base.h"
#include "unit_neopixel.h"
#include "ws2812.h"

/** Private data structure */
struct priv {
    char port_name;
    uint8_t pin_number;
    uint16_t pixels;

    uint32_t ll_pin;
    GPIO_TypeDef *port;
};

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
static void Npx_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    priv->port_name = pp_char(pp);
    priv->pin_number = pp_u8(pp);
    priv->pixels = pp_u16(pp);
}

/** Write to a binary buffer for storing in Flash */
static void Npx_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_char(pb, priv->port_name);
    pb_u8(pb, priv->pin_number);
    pb_u16(pb, priv->pixels);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
static error_t Npx_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "pin")) {
        suc = parse_pin(value, &priv->port_name, &priv->pin_number);
    }
    else if (streq(key, "pixels")) {
        priv->pixels = (uint16_t) avr_atoi(value);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
static void Npx_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Data pin");
    iw_entry(iw, "pin", "%c%d", priv->port_name,  priv->pin_number);

    iw_comment(iw, "Number of pixels");
    iw_entry(iw, "pixels", "%d", priv->pixels);
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static error_t Npx_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL)

        // some defaults
    priv->pin_number = 0;
    priv->port_name = 'A';
    priv->pixels = 1;

    return E_SUCCESS;
}

/** Finalize unit set-up */
static error_t Npx_init(Unit *unit)
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

    // clear strip

    ws2812_clear(priv->port, priv->ll_pin, priv->pixels);

    return E_SUCCESS;
}

/** Tear down the unit */
static void Npx_deInit(Unit *unit)
{
    // pins are de-inited during teardown

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}

// ------------------------------------------------------------------------

/* Clear the strip */
error_t UU_NEOPIXEL_Clear(Unit *unit)
{
    CHECK_TYPE(unit, &UNIT_NEOPIXEL);

    struct priv *priv = unit->data;
    ws2812_clear(priv->port, priv->ll_pin, priv->pixels);
    return E_SUCCESS;
}

/* Load packed */
error_t UU_NEOPIXEL_Load(Unit *unit, const uint8_t *packed_rgb, uint32_t nbytes)
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
inline error_t UU_NEOPIXEL_LoadU32LE(Unit *unit, const uint8_t *bytes, uint32_t nbytes)
{
    return load_u32(unit, bytes, nbytes, false);
}

/* Load U32, BE */
inline error_t UU_NEOPIXEL_LoadU32BE(Unit *unit, const uint8_t *bytes, uint32_t nbytes)
{
    return load_u32(unit, bytes, nbytes, true);
}

/* Get the pixel count */
error_t UU_NEOPIXEL_GetCount(Unit *unit, uint16_t *count)
{
    CHECK_TYPE(unit, &UNIT_NEOPIXEL);

    struct priv *priv = unit->data;
    *count = priv->pixels;
    return E_SUCCESS;
}

enum PinCmd_ {
    CMD_CLEAR = 0,
    CMD_LOAD = 1,
    CMD_LOAD_U32_LE = 2,
    CMD_LOAD_U32_BE = 3,
    CMD_GET_LEN = 4,
};

/** Handle a request message */
static error_t Npx_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    uint32_t len;
    const uint8_t *bytes;

    switch (command) {
        /** Clear the entire strip */
        case CMD_CLEAR:
            return UU_NEOPIXEL_Clear(unit);

        /** Load packed RGB colors (length must match the strip size) */
        case CMD_LOAD:;
            bytes = pp_tail(pp, &len);
            return UU_NEOPIXEL_Load(unit, bytes, len);

        /** Load sparse (uint32_t) colors, 0x00RRGGBB, little endian. */
        case CMD_LOAD_U32_LE:
            bytes = pp_tail(pp, &len);
            return UU_NEOPIXEL_LoadU32LE(unit, bytes, len);

        /** Load sparse (uint32_t) colors, 0x00RRGGBB, big endian. */
        case CMD_LOAD_U32_BE:
            bytes = pp_tail(pp, &len);
            return UU_NEOPIXEL_LoadU32BE(unit, bytes, len);

        /** Get the Neopixel strip length */
        case CMD_GET_LEN:;
            uint16_t count;
            TRY(UU_NEOPIXEL_GetCount(unit, &count));
            com_respond_u16(frame_id, count);
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_NEOPIXEL = {
    .name = "NEOPIXEL",
    .description = "Neopixel RGB LED strip",
    // Settings
    .preInit = Npx_preInit,
    .cfgLoadBinary = Npx_loadBinary,
    .cfgWriteBinary = Npx_writeBinary,
    .cfgLoadIni = Npx_loadIni,
    .cfgWriteIni = Npx_writeIni,
    // Init
    .init = Npx_init,
    .deInit = Npx_deInit,
    // Function
    .handleRequest = Npx_handleRequest,
};
