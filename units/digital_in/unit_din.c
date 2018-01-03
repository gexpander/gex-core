//
// Created by MightyPork on 2017/11/25.
//

#include <stm32f072xb.h>
#include "comm/messages.h"
#include "unit_base.h"
#include "unit_din.h"

/** Private data structure */
struct priv {
    char port_name;
    uint16_t pins; // pin mask
    uint16_t pulldown; // pull-downs (default is pull-up)
    uint16_t pullup; // pull-ups

    GPIO_TypeDef *port;
};

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
static void DI_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->port_name = pp_char(pp);
    priv->pins = pp_u16(pp);
    priv->pulldown = pp_u16(pp);
    priv->pullup = pp_u16(pp);

    dbg("load bin, pulldown = %X", priv->pulldown);
}

/** Write to a binary buffer for storing in Flash */
static void DI_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_char(pb, priv->port_name);
    pb_u16(pb, priv->pins);
    pb_u16(pb, priv->pulldown);
    pb_u16(pb, priv->pullup);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
static bool DI_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "port")) {
        suc = parse_port(value, &priv->port_name);
    }
    else if (streq(key, "pins")) {
        priv->pins = parse_pinmask(value, &suc);
    }
    else if (streq(key, "pull-up")) {
        priv->pullup = parse_pinmask(value, &suc);
    }
    else if (streq(key, "pull-down")) {
        priv->pulldown = parse_pinmask(value, &suc);
        dbg("parsinf ini, pulldown = %X", priv->pulldown);
    }
    else {
        return false;
    }

    return suc;
}

/** Generate INI file section for the unit */
static void DI_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Port name");
    iw_entry(iw, "port", "%c", priv->port_name);

    iw_comment(iw, "Pins (comma separated, supports ranges)");
    iw_entry(iw, "pins", "%s", str_pinmask(priv->pins, unit_tmp512));

    iw_comment(iw, "Pins with pull-up");
    iw_entry(iw, "pull-up", "%s", str_pinmask(priv->pullup, unit_tmp512));

    iw_comment(iw, "Pins with pull-down");
    iw_entry(iw, "pull-down", "%s", str_pinmask(priv->pulldown, unit_tmp512));

#if PLAT_NO_FLOATING_INPUTS
    iw_comment(iw, "NOTE: Pins use pull-up by default.\r\n");
#endif
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static bool DI_preInit(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv), &suc);
    CHECK_SUC();

    // some defaults
    priv->port_name = 'A';
    priv->pins = 0x0001;
    priv->pulldown = 0x0000;
    priv->pullup = 0x0000;

    return true;
}

/** Finalize unit set-up */
static bool DI_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    priv->pulldown &= priv->pins;
    priv->pullup &= priv->pins;

    // --- Parse config ---
    priv->port = port2periph(priv->port_name, &suc);
    if (!suc) {
        unit->status = E_BAD_CONFIG;
        return false;
    }

    // Claim all needed pins
    suc = rsc_claim_gpios(unit, priv->port_name, priv->pins);
    CHECK_SUC();

    uint16_t mask = 1;
    for (int i = 0; i < 16; i++, mask <<= 1) {
        if (priv->pins & mask) {
            uint32_t ll_pin = pin2ll((uint8_t) i, &suc);

            // --- Init hardware ---
            LL_GPIO_SetPinMode(priv->port, ll_pin, LL_GPIO_MODE_INPUT);

            uint32_t pull = 0;

            #if PLAT_NO_FLOATING_INPUTS
                pull = LL_GPIO_PULL_UP;
            #else
                pull = LL_GPIO_PULL_NO;
            #endif

            if (priv->pulldown & mask) pull = LL_GPIO_PULL_DOWN;
            if (priv->pullup & mask) pull = LL_GPIO_PULL_UP;
            LL_GPIO_SetPinPull(priv->port, ll_pin, pull);
        }
    }

    return true;
}


/** Tear down the unit */
static void DI_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    if (unit->status == E_SUCCESS) {
        bool suc = true;
        uint16_t mask = 1;
        for (int i = 0; i < 16; i++, mask <<= 1) {
            if (priv->pins & mask) {
                uint32_t ll_pin = pin2ll((uint8_t) i, &suc);
                assert_param(suc); // this should never fail if we got this far

                // configure the pin as analog
                LL_GPIO_SetPinMode(priv->port, ll_pin, LL_GPIO_MODE_ANALOG);
            }
        }
    }

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free(unit->data);
    unit->data = NULL;
}

// ------------------------------------------------------------------------

enum PinCmd_ {
    CMD_READ = 0,
};

/** Handle a request message */
static bool DI_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    (void)pp;

    struct priv *priv = unit->data;

    uint16_t packed = port_pack((uint16_t) priv->port->IDR, priv->pins);

    switch (command) {
        case CMD_READ:;
            PayloadBuilder pb = pb_start((uint8_t*)unit_tmp512, 64, NULL);
            pb_u16(&pb, packed);
            com_respond_buf(frame_id, MSG_SUCCESS, (uint8_t *) unit_tmp512, pb_length(&pb));
            break;

        default:
            com_respond_bad_cmd(frame_id);
            return false;
    }

    return true;
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_DIN = {
    .name = "DI",
    .description = "Digital input",
    // Settings
    .preInit = DI_preInit,
    .cfgLoadBinary = DI_loadBinary,
    .cfgWriteBinary = DI_writeBinary,
    .cfgLoadIni = DI_loadIni,
    .cfgWriteIni = DI_writeIni,
    // Init
    .init = DI_init,
    .deInit = DI_deInit,
    // Function
    .handleRequest = DI_handleRequest,
};
