//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define TPL_INTERNAL
#include "_tpl_internal.h"

/** Load from a binary buffer stored in Flash */
void UTPL_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    //
}

/** Write to a binary buffer for storing in Flash */
void UTPL_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    //
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UTPL_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (false) {
        //
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void UTPL_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    //
}

