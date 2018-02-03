//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define NPX_INTERNAL
#include "_npx_internal.h"
#include "_npx_settings.h"

/** Load from a binary buffer stored in Flash */
void Npx_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    priv->port_name = pp_char(pp);
    priv->pin_number = pp_u8(pp);
    priv->pixels = pp_u16(pp);
}

/** Write to a binary buffer for storing in Flash */
void Npx_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_char(pb, priv->port_name);
    pb_u8(pb, priv->pin_number);
    pb_u16(pb, priv->pixels);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t Npx_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "pin")) {
        suc = parse_pin(value, &priv->port_name, &priv->pin_number);
    }
    else if (streq(key, "pixels")) {
        priv->pixels = (uint16_t) avr_atoi(value);
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
    iw_entry(iw, "pin", "%c%d", priv->port_name,  priv->pin_number);

    iw_comment(iw, "Number of pixels");
    iw_entry(iw, "pixels", "%d", priv->pixels);
}
