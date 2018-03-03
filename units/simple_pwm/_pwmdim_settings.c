//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define PWMDIM_INTERNAL
#include "_pwmdim_internal.h"

/** Load from a binary buffer stored in Flash */
void UPWMDIM_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->cfg.freq = pp_u32(pp);
    priv->cfg.ch1_choice = pp_u8(pp);
    priv->cfg.ch2_choice = pp_u8(pp);
    priv->cfg.ch3_choice = pp_u8(pp);
    priv->cfg.ch4_choice = pp_u8(pp);
}

/** Write to a binary buffer for storing in Flash */
void UPWMDIM_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_u32(pb, priv->cfg.freq);
    pb_u8(pb, priv->cfg.ch1_choice);
    pb_u8(pb, priv->cfg.ch2_choice);
    pb_u8(pb, priv->cfg.ch3_choice);
    pb_u8(pb, priv->cfg.ch4_choice);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UPWMDIM_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "frequency")) {
        priv->cfg.freq = cfg_u32_parse(value, &suc);
    }
    else if (streq(key, "ch1_pin")) {
        priv->cfg.ch1_choice = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "ch2_pin")) {
        priv->cfg.ch2_choice = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "ch3_pin")) {
        priv->cfg.ch3_choice = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "ch4_pin")) {
        priv->cfg.ch4_choice = cfg_u8_parse(value, &suc);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void UPWMDIM_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Default pulse frequency (Hz)");
    iw_entry_d(iw, "frequency", priv->cfg.freq);

    iw_comment(iw, "Pin mapping - 0=disabled");
    iw_comment(iw, "Channel1 - 1:PA6, 2:PB4, 3:PC6");
    iw_entry_d(iw, "ch1_pin", priv->cfg.ch1_choice);
    iw_comment(iw, "Channel2 - 1:PA7, 2:PB5, 3:PC7");
    iw_entry_d(iw, "ch2_pin", priv->cfg.ch2_choice);
    iw_comment(iw, "Channel3 - 1:PB0, 2:PC8");
    iw_entry_d(iw, "ch3_pin", priv->cfg.ch3_choice);
    iw_comment(iw, "Channel4 - 1:PB1, 2:PC9");
    iw_entry_d(iw, "ch4_pin", priv->cfg.ch4_choice);
}

