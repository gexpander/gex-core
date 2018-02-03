//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define TPL_INTERNAL
#include "_tpl_internal.h"
#include "_tpl_settings.h"

/** Load from a binary buffer stored in Flash */
void TPL_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    //
}

/** Write to a binary buffer for storing in Flash */
void TPL_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    //
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t TPL_loadIni(Unit *unit, const char *key, const char *value)
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
void TPL_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    //
}

