//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define NPX_INTERNAL
#include "_npx_internal.h"

/** Load from a binary buffer stored in Flash */
void Npx_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    priv->cfg.pin = (Resource) pp_u8(pp);
    priv->cfg.pixels = pp_u16(pp);
}

/** Write to a binary buffer for storing in Flash */
void Npx_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, priv->cfg.pin);
    pb_u16(pb, priv->cfg.pixels);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t Npx_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "pin")) {
        priv->cfg.pin = cfg_pinrsc_parse(value, &suc);
    }
    else if (streq(key, "pixels")) {
        priv->cfg.pixels = cfg_u16_parse(value, &suc);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void Npx_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Data pin");
    iw_entry_s(iw, "pin", cfg_pinrsc_encode(priv->cfg.pin));

    iw_comment(iw, "Number of pixels");
    iw_entry_d(iw, "pixels", priv->cfg.pixels);
}
