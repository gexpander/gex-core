//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define OW_INTERNAL
#include "_ow_internal.h"

/** Load from a binary buffer stored in Flash */
void OW_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->port_name = pp_char(pp);
    priv->pin_number = pp_u8(pp);
    priv->parasitic = pp_bool(pp);
}

/** Write to a binary buffer for storing in Flash */
void OW_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_char(pb, priv->port_name);
    pb_u8(pb, priv->pin_number);
    pb_bool(pb, priv->parasitic);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t OW_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "pin")) {
        suc = cfg_portpin_parse(value, &priv->port_name, &priv->pin_number);
    }
    else if (streq(key, "parasitic")) {
        priv->parasitic = cfg_bool_parse(value, &suc);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void OW_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Data pin");
    iw_entry(iw, "pin", "%c%d", priv->port_name,  priv->pin_number);

    iw_comment(iw, "Parasitic (bus-powered) mode");
    iw_entry_s(iw, "parasitic", str_yn(priv->parasitic));
}
