//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define I2C_INTERNAL
#include "_i2c_internal.h"

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
    priv->remap = pp_u8(pp);
}

/** Write to a binary buffer for storing in Flash */
void UI2C_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

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
        priv->periph_num = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "remap")) {
        priv->remap = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "analog-filter")) {
        priv->anf = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "digital-filter")) {
        priv->dnf = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "speed")) {
        priv->speed = cfg_u8_parse(value, &suc);
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
    iw_entry_d(iw, "device", priv->periph_num);

    iw_comment(iw, "Pin mappings (SCL,SDA)");
#if STM32F072xB
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
    iw_entry_d(iw, "remap", priv->remap);

    iw_cmt_newline(iw);
    iw_comment(iw, "Speed: 1-Standard, 2-Fast, 3-Fast+");
    iw_entry_d(iw, "speed", priv->speed);

    iw_comment(iw, "Analog noise filter enable (Y,N)");
    iw_entry_s(iw, "analog-filter", str_yn(priv->anf));

    iw_comment(iw, "Digital noise filter bandwidth (0-15)");
    iw_entry_d(iw, "digital-filter", priv->dnf);
}
