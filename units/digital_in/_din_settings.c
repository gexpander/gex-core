//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define DIN_INTERNAL
#include "_din_internal.h"

/** Load from a binary buffer stored in Flash */
void DIn_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->port_name = pp_char(pp);
    priv->pins = pp_u16(pp);
    priv->pulldown = pp_u16(pp);
    priv->pullup = pp_u16(pp);

    if (version >= 1) {
        priv->trig_rise = pp_u16(pp);
        priv->trig_fall = pp_u16(pp);
        priv->trig_holdoff = pp_u16(pp);
    }
    if (version >= 2) {
        priv->def_auto = pp_u16(pp);
    }
}

/** Write to a binary buffer for storing in Flash */
void DIn_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 2); // version

    pb_char(pb, priv->port_name);
    pb_u16(pb, priv->pins);
    pb_u16(pb, priv->pulldown);
    pb_u16(pb, priv->pullup);
    pb_u16(pb, priv->trig_rise);
    pb_u16(pb, priv->trig_fall);
    pb_u16(pb, priv->trig_holdoff);
    pb_u16(pb, priv->def_auto);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t DIn_loadIni(Unit *unit, const char *key, const char *value)
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
    else if (streq(key, "trig-rise")) {
        priv->trig_rise = parse_pinmask(value, &suc);
    }
    else if (streq(key, "trig-fall")) {
        priv->trig_fall = parse_pinmask(value, &suc);
    }
    else if (streq(key, "auto-trigger")) {
        priv->def_auto = parse_pinmask(value, &suc);
    }
    else if (streq(key, "hold-off")) {
        priv->trig_holdoff = (uint16_t) avr_atoi(value);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void DIn_writeIni(Unit *unit, IniWriter *iw)
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

    iw_cmt_newline(iw);
    iw_comment(iw, "Trigger pins activated by rising/falling edge");
    iw_entry(iw, "trig-rise", "%s", pinmask2str(priv->trig_rise, unit_tmp512));
    iw_entry(iw, "trig-fall", "%s", pinmask2str(priv->trig_fall, unit_tmp512));

    iw_comment(iw, "Trigger pins auto-armed by default");
    iw_entry(iw, "auto-trigger", "%s", pinmask2str(priv->def_auto, unit_tmp512));

    iw_comment(iw, "Triggers hold-off time (ms)");
    iw_entry(iw, "hold-off", "%d", (int)priv->trig_holdoff);

#if PLAT_NO_FLOATING_INPUTS
    iw_comment(iw, "NOTE: Pins use pull-up by default.\r\n");
#endif
}

