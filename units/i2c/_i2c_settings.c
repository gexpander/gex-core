//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define I2C_INTERNAL
#include "_i2c_internal.h"
#include "_i2c_settings.h"

/** Load from a binary buffer stored in Flash */
void UI2C_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->periph_num = pp_u8(pp);
    priv->anf = pp_bool(pp);
    priv->dnf = pp_u8(pp);
    priv->speed = pp_u8(pp);

    if (version >= 1) {
        priv->remap = pp_u8(pp);
    }
}

/** Write to a binary buffer for storing in Flash */
void UI2C_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 1); // version

    pb_u8(pb, priv->periph_num);
    pb_bool(pb, priv->anf);
    pb_u8(pb, priv->dnf);
    pb_u8(pb, priv->speed);
    pb_u8(pb, priv->remap);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UI2C_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "device")) {
        priv->periph_num = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "remap")) {
        priv->remap = (uint8_t) avr_atoi(value);
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
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void UI2C_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Peripheral number (I2Cx)");
    iw_entry(iw, "device", "%d", (int)priv->periph_num);

    iw_comment(iw, "Pin mappings (SCL,SDA)");
#if GEX_PLAT_F072_DISCOVERY
    iw_comment(iw, " I2C1: (0) B6,B7    (1) B8,B9");
    iw_comment(iw, " I2C2: (0) B10,B11  (1) B13,B14");
#elif GEX_PLAT_F103_BLUEPILL
    #error "NO IMPL"
#elif GEX_PLAT_F303_DISCOVERY
    #error "NO IMPL"
#elif GEX_PLAT_F407_DISCOVERY
    #error "NO IMPL"
#else
    #error "BAD PLATFORM!"
#endif
    iw_entry(iw, "remap", "%d", (int)priv->remap);

    iw_cmt_newline(iw);
    iw_comment(iw, "Speed: 1-Standard, 2-Fast, 3-Fast+");
    iw_entry(iw, "speed", "%d", (int)priv->speed);

    iw_comment(iw, "Analog noise filter enable (Y,N)");
    iw_entry(iw, "analog-filter", "%s", str_yn(priv->anf));

    iw_comment(iw, "Digital noise filter bandwidth (0-15)");
    iw_entry(iw, "digital-filter", "%d", (int)priv->dnf);
}
