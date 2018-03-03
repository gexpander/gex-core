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

    priv->conf.signal_pname = pp_char(pp);
    priv->conf.signal_pnum = pp_u8(pp);
    priv->conf.active_level = pp_bool(pp);
    priv->conf.dfilter = pp_u8(pp);
    priv->conf.direct_presc = pp_u8(pp);
    priv->conf.direct_msec = pp_u16(pp);
    priv->conf.startmode = (enum fcap_opmode) pp_u8(pp);
}

/** Write to a binary buffer for storing in Flash */
void UFCAP_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_char(pb, priv->conf.signal_pname);
    pb_u8(pb, priv->conf.signal_pnum);
    pb_bool(pb, priv->conf.active_level);
    pb_u8(pb, priv->conf.dfilter);
    pb_u8(pb, priv->conf.direct_presc);
    pb_u16(pb, priv->conf.direct_msec);
    pb_u8(pb, priv->conf.startmode);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UFCAP_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "pin")) {
        suc = cfg_portpin_parse(value, &priv->conf.signal_pname, &priv->conf.signal_pnum);
    }
    else if (streq(key, "active-level")) {
        priv->conf.active_level = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "input-filter")) {
        priv->conf.dfilter = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "direct-presc")) {
        priv->conf.direct_presc = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "direct-time")) {
        priv->conf.direct_msec = cfg_u16_parse(value, &suc);
    }
    else if (streq(key, "initial-mode")) {
        priv->conf.startmode = (enum fcap_opmode) cfg_enum4_parse(value,
                                                                  "N", OPMODE_IDLE,
                                                                  "I", OPMODE_INDIRECT_CONT,
                                                                  "D", OPMODE_DIRECT_CONT,
                                                                  "F", OPMODE_FREE_COUNTER,
                                                                  &suc);
    }
    else{
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
    iw_entry(iw, "pin", "%c%d", priv->conf.signal_pname,  priv->conf.signal_pnum);
    iw_cmt_newline(iw);

    iw_comment(iw, "Active level or edge (0-low,falling; 1-high,rising)");
    iw_entry_d(iw, "active-level", priv->conf.active_level);

    iw_comment(iw, "Input filtering (0-15)");
    iw_entry_d(iw, "input-filter", priv->conf.dfilter);

    iw_comment(iw, "Pulse counter pre-divider (1,2,4,8)");
    iw_entry_d(iw, "direct-presc", priv->conf.direct_presc);

    iw_comment(iw, "Pulse counting interval (ms)");
    iw_entry_d(iw, "direct-time", priv->conf.direct_msec);
    iw_cmt_newline(iw);

    iw_comment(iw, "Mode on startup: N-none, I-indirect, D-direct, F-free count");
    iw_entry_s(iw, "initial-mode", cfg_enum4_encode(priv->conf.startmode,
                                                  OPMODE_IDLE, "N",
                                                  OPMODE_INDIRECT_CONT, "I",
                                                  OPMODE_DIRECT_CONT, "D",
                                                  OPMODE_FREE_COUNTER, "F"));
}

