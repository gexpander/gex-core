//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define FCAP_INTERNAL
#include "_fcap_internal.h"

/** Load from a binary buffer stored in Flash */
void UFCAP_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->signal_pname = pp_char(pp);
    priv->signal_pnum = pp_u8(pp);
}

/** Write to a binary buffer for storing in Flash */
void UFCAP_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_char(pb, priv->signal_pname);
    pb_u8(pb, priv->signal_pnum);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UFCAP_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "signal-pin")) {
        suc = parse_pin(value, &priv->signal_pname, &priv->signal_pnum);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void UFCAP_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Signal input pin - one of:");
    iw_comment(iw, " Full support:  A0, A5, A15");
    iw_comment(iw, " Indirect only: A1, B3");
    iw_entry(iw, "signal-pin", "%c%d", priv->signal_pname,  priv->signal_pnum);
}

