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
    priv->trig_rise = pp_u16(pp);
    priv->trig_fall = pp_u16(pp);
    priv->trig_holdoff = pp_u16(pp);
    priv->def_auto = pp_u16(pp);
}

/** Write to a binary buffer for storing in Flash */
void DIn_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

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
        suc = cfg_port_parse(value, &priv->port_name);
    }
    else if (streq(key, "pins")) {
        priv->pins = cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "pull-up")) {
        priv->pullup = cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "pull-down")) {
        priv->pulldown = cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "trig-rise")) {
        priv->trig_rise = cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "trig-fall")) {
        priv->trig_fall = cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "auto-trigger")) {
        priv->def_auto = cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "hold-off")) {
        priv->trig_holdoff = cfg_u16_parse(value, &suc);
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
    iw_entry_s(iw, "pins", cfg_pinmask_encode(priv->pins, unit_tmp512, 0));

    iw_comment(iw, "Pins with pull-up");
    iw_entry_s(iw, "pull-up", cfg_pinmask_encode(priv->pullup, unit_tmp512, 0));

    iw_comment(iw, "Pins with pull-down");
    iw_entry_s(iw, "pull-down", cfg_pinmask_encode(priv->pulldown, unit_tmp512, 0));

    iw_cmt_newline(iw);
    iw_comment(iw, "Trigger pins activated by rising/falling edge");
    iw_entry_s(iw, "trig-rise", cfg_pinmask_encode(priv->trig_rise, unit_tmp512, 0));
    iw_entry_s(iw, "trig-fall", cfg_pinmask_encode(priv->trig_fall, unit_tmp512, 0));

    iw_comment(iw, "Trigger pins auto-armed by default");
    iw_entry_s(iw, "auto-trigger", cfg_pinmask_encode(priv->def_auto, unit_tmp512, 0));

    iw_comment(iw, "Triggers hold-off time (ms)");
    iw_entry_d(iw, "hold-off", priv->trig_holdoff);

#if PLAT_NO_FLOATING_INPUTS
    iw_comment(iw, "NOTE: Pins use pull-up by default.\r\n");
#endif
}

