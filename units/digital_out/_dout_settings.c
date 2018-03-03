//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define DOUT_INTERNAL
#include "_dout_internal.h"

/** Load from a binary buffer stored in Flash */
void DOut_loadBinary(Unit *unit, PayloadParser *pp)
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
void DOut_writeBinary(Unit *unit, PayloadBuilder *pb)
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
error_t DOut_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "port")) {
        suc = cfg_port_parse(value, &priv->port_name);
    }
    else if (streq(key, "pins")) {
        priv->pins = cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "initial")) {
        priv->initial = cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "open-drain")) {
        priv->open_drain = cfg_pinmask_parse(value, &suc);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void DOut_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Port name");
    iw_entry(iw, "port", "%c", priv->port_name);

    iw_comment(iw, "Pins (comma separated, supports ranges)");
    iw_entry_s(iw, "pins", cfg_pinmask_encode(priv->pins, unit_tmp512, 0));

    iw_comment(iw, "Initially high pins");
    iw_entry_s(iw, "initial", cfg_pinmask_encode(priv->initial, unit_tmp512, 0));

    iw_comment(iw, "Open-drain pins");
    iw_entry_s(iw, "open-drain", cfg_pinmask_encode(priv->open_drain, unit_tmp512, 0));
}
