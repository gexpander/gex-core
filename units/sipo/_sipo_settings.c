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

    priv->cfg.pin_store = (Resource) pp_u8(pp);
    priv->cfg.store_pol = pp_bool(pp);

    priv->cfg.pin_shift = (Resource) pp_u8(pp);
    priv->cfg.shift_pol = pp_bool(pp);

    priv->cfg.pin_clear = (Resource) pp_u8(pp);
    priv->cfg.clear_pol = pp_bool(pp);

    priv->cfg.data_pname = pp_char(pp);
    priv->cfg.data_pins = pp_u16(pp);
}

/** Write to a binary buffer for storing in Flash */
void USIPO_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_u8(pb, priv->cfg.pin_store);
    pb_bool(pb, priv->cfg.store_pol);

    pb_u8(pb, priv->cfg.pin_shift);
    pb_bool(pb, priv->cfg.shift_pol);

    pb_u8(pb, priv->cfg.pin_clear);
    pb_bool(pb, priv->cfg.clear_pol);

    pb_char(pb, priv->cfg.data_pname);
    pb_u16(pb, priv->cfg.data_pins);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t USIPO_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "store-pin")) {
        priv->cfg.pin_store = cfg_pinrsc_parse(value, &suc);
    }
    else if (streq(key, "shift-pin")) {
        priv->cfg.pin_shift = cfg_pinrsc_parse(value, &suc);
    }
    else if (streq(key, "clear-pin")) {
        priv->cfg.pin_clear = cfg_pinrsc_parse(value, &suc);
    }

    else if (streq(key, "store-pol")) {
        priv->cfg.store_pol = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "shift-pol")) {
        priv->cfg.shift_pol = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "clear-pol")) {
        priv->cfg.clear_pol = cfg_bool_parse(value, &suc);
    }

    else if (streq(key, "data-port")) {
        suc = cfg_port_parse(value, &priv->cfg.data_pname);
    }
    else if (streq(key, "data-pins")) {
        priv->cfg.data_pins = cfg_pinmask_parse(value, &suc);
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
    iw_entry_s(iw, "shift-pin", cfg_pinrsc_encode(priv->cfg.pin_shift));
    iw_entry_d(iw, "shift-pol", priv->cfg.shift_pol);

    iw_comment(iw, "Store pin & its active edge");
    iw_entry_s(iw, "store-pin", cfg_pinrsc_encode(priv->cfg.pin_store));
    iw_entry_d(iw, "store-pol", priv->cfg.store_pol);

    iw_comment(iw, "Clear pin & its active level");
    iw_entry_s(iw, "clear-pin", cfg_pinrsc_encode(priv->cfg.pin_clear));
    iw_entry_d(iw, "clear-pol", priv->cfg.clear_pol);

    iw_comment(iw, "Data port and pins");
    iw_entry(iw, "data-port", "%c", priv->cfg.data_pname);
    iw_entry_s(iw, "data-pins", cfg_pinmask_encode(priv->cfg.data_pins, unit_tmp512, true));
}

