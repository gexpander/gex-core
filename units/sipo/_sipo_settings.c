//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define SIPO_INTERNAL
#include "_sipo_internal.h"

/** Load from a binary buffer stored in Flash */
void USIPO_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->store_pname = pp_char(pp);
    priv->store_pnum = pp_u8(pp);
    priv->store_pol = pp_bool(pp);

    priv->shift_pname = pp_char(pp);
    priv->shift_pnum = pp_u8(pp);
    priv->shift_pol = pp_bool(pp);

    priv->clear_pname = pp_char(pp);
    priv->clear_pnum = pp_u8(pp);
    priv->clear_pol = pp_bool(pp);

    priv->data_pname = pp_char(pp);
    priv->data_pins = pp_u16(pp);
}

/** Write to a binary buffer for storing in Flash */
void USIPO_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_char(pb, priv->store_pname);
    pb_u8(pb, priv->store_pnum);
    pb_bool(pb, priv->store_pol);

    pb_char(pb, priv->shift_pname);
    pb_u8(pb, priv->shift_pnum);
    pb_bool(pb, priv->shift_pol);

    pb_char(pb, priv->clear_pname);
    pb_u8(pb, priv->clear_pnum);
    pb_bool(pb, priv->clear_pol);

    pb_char(pb, priv->data_pname);
    pb_u16(pb, priv->data_pins);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t USIPO_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "store-pin")) {
        suc = cfg_portpin_parse(value, &priv->store_pname, &priv->store_pnum);
    }
    else if (streq(key, "shift-pin")) {
        suc = cfg_portpin_parse(value, &priv->shift_pname, &priv->shift_pnum);
    }
    else if (streq(key, "clear-pin")) {
        suc = cfg_portpin_parse(value, &priv->clear_pname, &priv->clear_pnum);
    }

    else if (streq(key, "store-pol")) {
        priv->store_pol = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "shift-pol")) {
        priv->shift_pol = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "clear-pol")) {
        priv->clear_pol = cfg_bool_parse(value, &suc);
    }

    else if (streq(key, "data-port")) {
        suc = cfg_port_parse(value, &priv->data_pname);
    }
    else if (streq(key, "data-pins")) {
        priv->data_pins = cfg_pinmask_parse(value, &suc);
    }

    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void USIPO_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Shift pin & its active edge (1-rising,0-falling)");
    iw_entry(iw, "shift-pin", "%c%d", priv->shift_pname,  priv->shift_pnum);
    iw_entry(iw, "shift-pol", "%d", priv->shift_pol);

    iw_comment(iw, "Store pin & its active edge");
    iw_entry(iw, "store-pin", "%c%d", priv->store_pname,  priv->store_pnum);
    iw_entry(iw, "store-pol", "%d", priv->store_pol);

    iw_comment(iw, "Clear pin & its active level");
    iw_entry(iw, "clear-pin", "%c%d", priv->clear_pname,  priv->clear_pnum);
    iw_entry(iw, "clear-pol", "%d", priv->clear_pol);

    iw_comment(iw, "Data port and pins");
    iw_entry(iw, "data-port", "%c", priv->data_pname);
    iw_entry(iw, "data-pins", "%s", cfg_pinmask_encode(priv->data_pins, unit_tmp512, true));
}

