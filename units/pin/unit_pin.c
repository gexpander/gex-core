//
// Created by MightyPork on 2017/11/25.
//

#include "comm/messages.h"
#include "unit_base.h"
#include "unit_pin.h"

/** Private data structure */
struct priv {
    char port_name;
    uint8_t pin_number;
    bool output;
    bool pull_up;
    bool open_drain;

    uint32_t ll_pin;
    GPIO_TypeDef *port;
};

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
static void Pin_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    priv->port_name = pp_char(pp);
    priv->pin_number = pp_u8(pp);
    priv->output = pp_bool(pp);
    priv->pull_up = pp_bool(pp);
    priv->open_drain = pp_bool(pp);
}

/** Write to a binary buffer for storing in Flash */
static void Pin_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_char(pb, priv->port_name);
    pb_u8(pb, priv->pin_number);
    pb_bool(pb, priv->output);
    pb_bool(pb, priv->pull_up);
    pb_bool(pb, priv->open_drain);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
static bool Pin_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "pin")) {
        suc = str_parse_pin(value, &priv->port_name, &priv->pin_number);
    }
    else if (streq(key, "dir")) {
        priv->output = str_parse_01(value, "IN", "OUT", &suc);
    }
    else if (streq(key, "pull")) {
        priv->pull_up = str_parse_01(value, "DOWN", "UP", &suc);
    }
    else if (streq(key, "opendrain")) {
        priv->open_drain = str_parse_yn(value, &suc);
    } else {
        return false;
    }

    return suc;
}

/** Generate INI file section for the unit */
static void Pin_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Physical pin");
    iw_entry(iw, "pin", "%c%d", priv->port_name,  priv->pin_number);

    iw_comment(iw, "Direction (IN, OUT)");
    iw_entry(iw, "dir", "%s", str_01(priv->output,  "IN",   "OUT"));

    iw_comment(iw, "Pull resistor, only for input (UP, DOWN)");
    iw_entry(iw, "pull", "%s", str_01(priv->pull_up, "DOWN", "UP"));

    iw_comment(iw, "Open drain, only for output (Y, N)");
    iw_entry(iw, "opendrain", "%s", str_yn(priv->open_drain));
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static bool Pin_preInit(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv), &suc);
    CHECK_SUC();

    // some defaults
    priv->pin_number = 0;
    priv->port_name = 'A';
    priv->output = true;
    priv->open_drain = false;
    priv->pull_up = false;

    return true;
}

/** Finalize unit set-up */
static bool Pin_init(Unit *unit)
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
    LL_GPIO_SetPinMode(priv->port, priv->ll_pin,
        priv->output ? LL_GPIO_MODE_OUTPUT : LL_GPIO_MODE_INPUT);

    LL_GPIO_SetPinOutputType(priv->port, priv->ll_pin,
        priv->open_drain ? LL_GPIO_OUTPUT_OPENDRAIN : LL_GPIO_OUTPUT_PUSHPULL);

    LL_GPIO_SetPinPull(priv->port, priv->ll_pin,
        priv->pull_up ? LL_GPIO_PULL_UP : LL_GPIO_PULL_DOWN);

    LL_GPIO_SetPinSpeed(priv->port, priv->ll_pin,
        LL_GPIO_SPEED_FREQ_HIGH);

    return true;
}

/** Tear down the unit */
static void Pin_deInit(Unit *unit)
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
    CMD_SET = 1,
    CMD_TOGGLE = 2,
    CMD_READ = 3,
};

/** Handle a request message */
static bool Pin_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    (void)pp;

    struct priv *priv = unit->data;

    switch (command) {
        case CMD_CLEAR:
            if (priv->output) {
                LL_GPIO_ResetOutputPin(priv->port, priv->ll_pin);
            } else goto must_be_output;
            break;

        case CMD_SET:
            if (priv->output) {
                LL_GPIO_SetOutputPin(priv->port, priv->ll_pin);
            } else goto must_be_output;
            break;

        case CMD_TOGGLE:
            if (priv->output) {
                LL_GPIO_TogglePin(priv->port, priv->ll_pin);
            } else goto must_be_output;
            break;

        case CMD_READ:
            if (!priv->output) {
                com_respond_u8(frame_id, (bool) LL_GPIO_IsInputPinSet(priv->port, priv->ll_pin));
            } else goto must_be_input;
            break;

        default:
            com_respond_bad_cmd(frame_id);
            return false;
    }

    return true;

must_be_output:
    com_respond_err(frame_id, "NOT OUTPUT PIN");
    return false;

must_be_input:
    com_respond_err(frame_id, "NOT INPUT PIN");
    return false;
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_PIN = {
    .name = "PIN",
    .description = "Single digital I/O pin",
    // Settings
    .preInit = Pin_preInit,
    .cfgLoadBinary = Pin_loadBinary,
    .cfgWriteBinary = Pin_writeBinary,
    .cfgLoadIni = Pin_loadIni,
    .cfgWriteIni = Pin_writeIni,
    // Init
    .init = Pin_init,
    .deInit = Pin_deInit,
    // Function
    .handleRequest = Pin_handleRequest,
};
