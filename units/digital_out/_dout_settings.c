//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define DOUT_INTERNAL
#include "_dout_internal.h"
#include "_dout_settings.h"

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
        suc = parse_port_name(value, &priv->port_name);
    }
    else if (streq(key, "pins")) {
        priv->pins = parse_pinmask(value, &suc);
    }
    else if (streq(key, "initial")) {
        priv->initial = parse_pinmask(value, &suc);
    }
    else if (streq(key, "open-drain")) {
        priv->open_drain = parse_pinmask(value, &suc);
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
    iw_entry(iw, "pins", "%s", pinmask2str(priv->pins, unit_tmp512));

    iw_comment(iw, "Initially high pins");
    iw_entry(iw, "initial", "%s", pinmask2str(priv->initial, unit_tmp512));

    iw_comment(iw, "Open-drain pins");
    iw_entry(iw, "open-drain", "%s", pinmask2str(priv->open_drain, unit_tmp512));
}
