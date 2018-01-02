//
// Created by MightyPork on 2017/11/25.
//

#include "comm/messages.h"
#include "unit_base.h"
#include "unit_dout.h"

/** Private data structure */
struct priv {
    char port_name;
    uint16_t pins; // pin mask
    uint16_t initial; // initial pin states
    uint16_t open_drain; // open drain pins

    GPIO_TypeDef *port;
};

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
static void DO_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->port_name = pp_char(pp);
    priv->pins = pp_u16(pp);
    priv->initial = pp_u16(pp);
    priv->open_drain = pp_u16(pp);
}

/** Write to a binary buffer for storing in Flash */
static void DO_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_char(pb, priv->port_name);
    pb_u16(pb, priv->pins);
    pb_u16(pb, priv->initial);
    pb_u16(pb, priv->open_drain);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
static bool DO_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "port")) {
        suc = parse_port(value, &priv->port_name);
    }
    else if (streq(key, "pins")) {
        priv->pins = parse_pinmask(value, &suc);
    }
    else if (streq(key, "initial")) {
        priv->initial = parse_pinmask(value, &suc);
    }
    else if (streq(key, "opendrain")) {
        priv->open_drain = parse_pinmask(value, &suc);
    }
    else {
        return false;
    }

    return suc;
}

/** Generate INI file section for the unit */
static void DO_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Port name");
    iw_entry(iw, "port", "%c", priv->port_name);

    iw_comment(iw, "Pins (comma separated, supports ranges)");
    iw_entry(iw, "pins", "%s", str_pinmask(priv->pins, unit_tmp64));

    iw_comment(iw, "Initially high pins");
    iw_entry(iw, "initial", "%s", str_pinmask(priv->initial, unit_tmp64));

    iw_comment(iw, "Open-drain pins");
    iw_entry(iw, "opendrain", "%s", str_pinmask(priv->open_drain, unit_tmp64));
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static bool DO_preInit(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv), &suc);
    CHECK_SUC();

    // some defaults
    priv->port_name = 'A';
    priv->pins = 0x0001;
    priv->open_drain = 0x0000;
    priv->initial = 0x0000;

    return true;
}

/** Finalize unit set-up */
static bool DO_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    priv->initial &= priv->pins;
    priv->open_drain &= priv->pins;

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
            LL_GPIO_SetPinMode(priv->port, ll_pin, LL_GPIO_MODE_OUTPUT);

            LL_GPIO_SetPinOutputType(priv->port, ll_pin, (priv->open_drain & mask) ?
                                                         LL_GPIO_OUTPUT_OPENDRAIN :
                                                         LL_GPIO_OUTPUT_PUSHPULL);

            LL_GPIO_SetPinSpeed(priv->port, ll_pin, LL_GPIO_SPEED_FREQ_HIGH);
        }
    }

    // Set the initial state
    priv->port->ODR &= ~priv->pins;
    priv->port->ODR |= priv->initial;

    return true;
}

/** Tear down the unit */
static void DO_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init the pins only if inited correctly
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
    CMD_WRITE = 0,
    CMD_SET = 1,
    CMD_CLEAR = 2,
    CMD_TOGGLE = 3,
};

/** Handle a request message */
static bool DO_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    (void)pp;

    struct priv *priv = unit->data;

    uint16_t packed = pp_u16(pp);

    uint16_t mask = priv->pins;
    uint16_t spread = port_spread(packed, mask);

    uint16_t set, reset;

    switch (command) {
        case CMD_WRITE:;
            set = spread;
            reset = (~spread) & mask;
            priv->port->BSRR = set | (reset<<16);
            break;

        case CMD_SET:
            priv->port->BSRR = spread;
            break;

        case CMD_CLEAR:
            priv->port->BSRR = (spread<<16);
            break;

        case CMD_TOGGLE:;
            spread = (uint16_t) (~priv->port->ODR) & mask;
            set = spread;
            reset = (~spread) & mask;
            priv->port->BSRR = set | (reset<<16);
            break;

        default:
            com_respond_bad_cmd(frame_id);
            return false;
    }

    return true;
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_DOUT = {
    .name = "DO",
    .description = "Digital output",
    // Settings
    .preInit = DO_preInit,
    .cfgLoadBinary = DO_loadBinary,
    .cfgWriteBinary = DO_writeBinary,
    .cfgLoadIni = DO_loadIni,
    .cfgWriteIni = DO_writeIni,
    // Init
    .init = DO_init,
    .deInit = DO_deInit,
    // Function
    .handleRequest = DO_handleRequest,
};
