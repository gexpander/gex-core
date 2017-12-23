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
static bool Npx_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "pin")) {
        suc = str_parse_pin(value, &priv->port_name, &priv->pin_number);
    }
    else if (streq(key, "pixels")) {
        priv->pixels = (uint16_t) avr_atoi(value);
    }
    else {
        return false;
    }

    return suc;
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
static bool Npx_preInit(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv), &suc);
    CHECK_SUC();

    // some defaults
    priv->pin_number = 0;
    priv->port_name = 'A';
    priv->pixels = 1;

    return true;
}

/** Finalize unit set-up */
static bool Npx_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    // --- Parse config ---
    priv->ll_pin = plat_pin2ll(priv->pin_number, &suc);
    priv->port = plat_port2periph(priv->port_name, &suc);
    Resource rsc = plat_pin2resource(priv->port_name, priv->pin_number, &suc);
    if (!suc) {
        unit->status = E_BAD_CONFIG;
        return false;
    }

    // --- Claim resources ---
    if (!rsc_claim(unit, rsc)) return false;

    // --- Init hardware ---
    LL_GPIO_SetPinMode(priv->port, priv->ll_pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(priv->port, priv->ll_pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(priv->port, priv->ll_pin, LL_GPIO_SPEED_FREQ_HIGH);

    // clear strip

    ws2812_clear(priv->port, priv->ll_pin, priv->pixels);

    return true;
}

/** Tear down the unit */
static void Npx_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // configure the pin as analog
    LL_GPIO_SetPinMode(priv->port, priv->ll_pin, LL_GPIO_MODE_ANALOG);

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free(unit->data);
    unit->data = NULL;
}

// ------------------------------------------------------------------------

enum PinCmd_ {
    CMD_CLEAR = 0,
    CMD_LOAD = 1,
    CMD_LOAD_U32_LE = 2,
    CMD_LOAD_U32_BE = 3,
};

/** Handle a request message */
static bool Npx_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    (void)pp;

    struct priv *priv = unit->data;

    switch (command) {
        case CMD_CLEAR:
            ws2812_clear(priv->port, priv->ll_pin, priv->pixels);
            break;

        case CMD_LOAD:
            if (pp_length(pp) != priv->pixels*3) goto bad_count;
            ws2812_load_raw(priv->port, priv->ll_pin, pp->current, priv->pixels);
            break;

        case CMD_LOAD_U32_LE:
            if (pp_length(pp) != priv->pixels*4) goto bad_count;
            ws2812_load_sparse(priv->port, priv->ll_pin, pp->current, priv->pixels, 0);
            break;

        case CMD_LOAD_U32_BE:
            if (pp_length(pp) != priv->pixels*4) goto bad_count;
            ws2812_load_sparse(priv->port, priv->ll_pin, pp->current, priv->pixels, 1);
            break;

        default:
            com_respond_bad_cmd(frame_id);
            return false;
    }

    return true;

bad_count:
    com_respond_err(frame_id, "BAD PIXEL COUNT");
    return false;
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
