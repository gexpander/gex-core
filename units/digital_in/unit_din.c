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
static error_t DI_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "port")) {
        suc = parse_port_name(value, &priv->port_name);
    }
    else if (streq(key, "pins")) {
        priv->pins = parse_pinmask(value, &suc);
    }
    else if (streq(key, "pull-up")) {
        priv->pullup = parse_pinmask(value, &suc);
    }
    else if (streq(key, "pull-down")) {
        priv->pulldown = parse_pinmask(value, &suc);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
static void DI_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Port name");
    iw_entry(iw, "port", "%c", priv->port_name);

    iw_comment(iw, "Pins (comma separated, supports ranges)");
    iw_entry(iw, "pins", "%s", pinmask2str(priv->pins, unit_tmp512));

    iw_comment(iw, "Pins with pull-up");
    iw_entry(iw, "pull-up", "%s", pinmask2str(priv->pullup, unit_tmp512));

    iw_comment(iw, "Pins with pull-down");
    iw_entry(iw, "pull-down", "%s", pinmask2str(priv->pulldown, unit_tmp512));

#if PLAT_NO_FLOATING_INPUTS
    iw_comment(iw, "NOTE: Pins use pull-up by default.\r\n");
#endif
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static error_t DI_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    // some defaults
    priv->port_name = 'A';
    priv->pins = 0x0001;
    priv->pulldown = 0x0000;
    priv->pullup = 0x0000;

    return E_SUCCESS;
}

/** Finalize unit set-up */
static error_t DI_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    priv->pulldown &= priv->pins;
    priv->pullup &= priv->pins;

    // --- Parse config ---
    priv->port = hw_port2periph(priv->port_name, &suc);
    if (!suc) return E_BAD_CONFIG;

    // Claim all needed pins
    TRY(rsc_claim_gpios(unit, priv->port_name, priv->pins));

    uint16_t mask = 1;
    for (int i = 0; i < 16; i++, mask <<= 1) {
        if (priv->pins & mask) {
            uint32_t ll_pin = hw_pin2ll((uint8_t) i, &suc);

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

    return E_SUCCESS;
}


/** Tear down the unit */
static void DI_deInit(Unit *unit)
{
    // pins are de-inited during teardown

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}

// ------------------------------------------------------------------------

/** Read request */
error_t UU_DI_Read(Unit *unit, uint16_t *packed)
{
    CHECK_TYPE(unit, &UNIT_DIN);
    struct priv *priv = unit->data;
    *packed = pinmask_pack((uint16_t) priv->port->IDR, priv->pins);
    return E_SUCCESS;
}

enum PinCmd_ {
    CMD_READ = 0,
};

/** Handle a request message */
static error_t DI_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    uint16_t packed = 0;

    switch (command) {
        case CMD_READ:;
            TRY(UU_DI_Read(unit, &packed));

            PayloadBuilder pb = pb_start((uint8_t*)unit_tmp512, UNIT_TMP_LEN, NULL);
            pb_u16(&pb, packed);
            com_respond_buf(frame_id, MSG_SUCCESS, (uint8_t *) unit_tmp512, pb_length(&pb));
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
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
