//
// Created by MightyPork on 2018/01/02.
//

#include "comm/messages.h"
#include "unit_base.h"
#include "utils/avrlibc.h"
#include "unit_i2c.h"

// I2C master

/** Private data structure */
struct priv {
    uint8_t periph_num; //!< 1 or 2
    bool anf;    //!< Enable analog noise filter
    uint8_t dnf; //!< Enable digital noise filter (1-15 ... max spike width)
    uint8_t speed; //!< 0 - Standard, 1 - Fast, 2 - Fast+

    I2C_TypeDef *periph;
};

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
static void UI2C_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->periph_num = pp_u8(pp);
    priv->anf = pp_bool(pp);
    priv->dnf = pp_u8(pp);
    priv->speed = pp_u8(pp);
}

/** Write to a binary buffer for storing in Flash */
static void UI2C_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_u8(pb, priv->periph_num);
    pb_bool(pb, priv->anf);
    pb_u8(pb, priv->dnf);
    pb_u8(pb, priv->speed);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
static bool UI2C_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "device")) {
        priv->periph_num = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "analog-filter")) {
        priv->anf = str_parse_yn(value, &suc);
    }
    else if (streq(key, "digital-filter")) {
        priv->dnf = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "speed")) {
        priv->speed = (uint8_t) avr_atoi(value);
    }
    else {
        return false;
    }

    return suc;
}

/** Generate INI file section for the unit */
static void UI2C_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Peripheral number (I2Cx)");
    iw_entry(iw, "device", "%d", (int)priv->periph_num);

    iw_comment(iw, "Speed: 1-Standard, 2-Fast, 3-Fast+");
    iw_entry(iw, "speed", "%d", (int)priv->speed);

    iw_comment(iw, "Analog noise filter enable (Y,N)");
    iw_entry(iw, "analog-filter", "%s", str_yn(priv->anf));

    iw_comment(iw, "Digital noise filter bandwidth (0-15)");
    iw_entry(iw, "digital-filter", "%d", (int)priv->dnf);
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static bool UI2C_preInit(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv), &suc);
    CHECK_SUC();

    // some defaults
    priv->periph_num = 1;
    priv->speed = 1;
    priv->anf = true;
    priv->dnf = 0;

    return true;
}

/** Finalize unit set-up */
static bool UI2C_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (!(priv->periph_num >= 1 && priv->periph_num <= 2)) {
        unit->status = E_BAD_CONFIG;
        dbg("!! Bad I2C periph");
        return false;
    }

    if (!(priv->speed >= 1 && priv->speed <= 3)) {
        unit->status = E_BAD_CONFIG;
        dbg("!! Bad I2C speed");
        return false;
    }

    if (priv->dnf > 15) {
        unit->status = E_BAD_CONFIG;
        dbg("!! Bad I2C DNF bw");
        return false;
    }

    // assign and claim the peripheral
    if (priv->periph_num == 1) {
        suc = rsc_claim(unit, R_I2C1);
        CHECK_SUC();
        priv->periph = I2C1;
    } else {
        suc = rsc_claim(unit, R_I2C2);
        CHECK_SUC();
        priv->periph = I2C2;
    }

    // TODO claim pins (config option to remap?)


    return true;
}

/** Tear down the unit */
static void UI2C_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init the pins & peripheral only if inited correctly
    if (unit->status == E_SUCCESS) {
        // TODO
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
    CMD_READ = 1,
};

/** Handle a request message */
static bool UI2C_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    switch (command) {
        case CMD_WRITE:
            //
            break;

        case CMD_READ:
            //
            break;

        default:
            com_respond_bad_cmd(frame_id);
            return false;
    }

    return true;
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_I2C = {
    .name = "I2C",
    .description = "I2C master",
    // Settings
    .preInit = UI2C_preInit,
    .cfgLoadBinary = UI2C_loadBinary,
    .cfgWriteBinary = UI2C_writeBinary,
    .cfgLoadIni = UI2C_loadIni,
    .cfgWriteIni = UI2C_writeIni,
    // Init
    .init = UI2C_init,
    .deInit = UI2C_deInit,
    // Function
    .handleRequest = UI2C_handleRequest,
};
