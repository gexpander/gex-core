//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define TOUCH_INTERNAL
#include "_touch_internal.h"

// some channels are mapped to nonexistent ports, so just ignore them - clutters the config
#define HAVE_CH7 0
#define HAVE_CH8 0

/** Load from a binary buffer stored in Flash */
void UTOUCH_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->cfg.charge_time = pp_u8(pp);
    priv->cfg.drain_time = pp_u8(pp);
    priv->cfg.spread_deviation = pp_u8(pp);
    priv->cfg.ss_presc = pp_u8(pp);
    priv->cfg.pg_presc = pp_u8(pp);
    priv->cfg.sense_timeout = pp_u8(pp);
    pp_buf(pp, priv->cfg.group_scaps, 8);
    pp_buf(pp, priv->cfg.group_channels, 8);
}

/** Write to a binary buffer for storing in Flash */
void UTOUCH_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_u8(pb, priv->cfg.charge_time);
    pb_u8(pb, priv->cfg.drain_time);
    pb_u8(pb, priv->cfg.spread_deviation);
    pb_u8(pb, priv->cfg.ss_presc);
    pb_u8(pb, priv->cfg.pg_presc);
    pb_u8(pb, priv->cfg.sense_timeout);
    pb_buf(pb, priv->cfg.group_scaps, 8);
    pb_buf(pb, priv->cfg.group_channels, 8);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UTOUCH_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "charge-time")) {
        priv->cfg.charge_time = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "drain-time")) {
        priv->cfg.drain_time = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "ss-deviation")) {
        priv->cfg.spread_deviation = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "ss-clock-prediv")) {
        priv->cfg.ss_presc = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "pg-clock-prediv")) {
        priv->cfg.pg_presc = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "sense-timeout")) {
        priv->cfg.sense_timeout = cfg_u8_parse(value, &suc);
    }

    else if (streq(key, "g1_cap")) {
        priv->cfg.group_scaps[0] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "g2_cap")) {
        priv->cfg.group_scaps[1] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "g3_cap")) {
        priv->cfg.group_scaps[2] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "g4_cap")) {
        priv->cfg.group_scaps[3] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "g5_cap")) {
        priv->cfg.group_scaps[4] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "g6_cap")) {
        priv->cfg.group_scaps[5] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
#if HAVE_CH7
    else if (streq(key, "g7_cap")) {
        priv->cfg.group_scaps[6] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
#endif
#if HAVE_CH8
    else if (streq(key, "g8_cap")) {
        priv->cfg.group_scaps[7] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
#endif

    else if (streq(key, "g1_ch")) {
        priv->cfg.group_channels[0] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "g2_ch")) {
        priv->cfg.group_channels[1] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "g3_ch")) {
        priv->cfg.group_channels[2] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "g4_ch")) {
        priv->cfg.group_channels[3] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "g5_ch")) {
        priv->cfg.group_channels[4] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
    else if (streq(key, "g6_ch")) {
        priv->cfg.group_channels[5] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
#if HAVE_CH7
    else if (streq(key, "g7_ch")) {
        priv->cfg.group_channels[6] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
#endif
#if HAVE_CH8
    else if (streq(key, "g8_ch")) {
        priv->cfg.group_channels[7] = (uint8_t) cfg_pinmask_parse(value, &suc);
    }
#endif

    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void UTOUCH_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "This unit utilizes the touch sensing controller.");
    iw_comment(iw, "See the reference manual for details about its function.");
    iw_cmt_newline(iw);

    iw_comment(iw, "Pulse generator clock prescaller (1,2,4,...,128)");
    iw_entry(iw, "pg-clock-prediv", "%d", (int)priv->cfg.ss_presc);
    iw_comment(iw, "Sense pad charging time (1-16)");
    iw_entry(iw, "charge-time", "%d", (int)priv->cfg.charge_time);
    iw_comment(iw, "Charge transfer time (1-16)");
    iw_entry(iw, "drain-time", "%d", (int)priv->cfg.drain_time);
    iw_comment(iw, "Spread spectrum max deviation (0-128,0=off)");
    iw_entry(iw, "ss-deviation", "%d", (int)priv->cfg.spread_deviation);
    iw_comment(iw, "Spreading clock prescaller (1,2)");
    iw_entry(iw, "ss-clock-prediv", "%d", (int)priv->cfg.ss_presc);
    iw_comment(iw, "Measurement timeout (1-7)");
    iw_entry(iw, "sense-timeout", "%d", (int)priv->cfg.sense_timeout);

    iw_cmt_newline(iw);
    iw_comment(iw, "Each used group must have 1 sampling capacitor and 1-3 channels.");
    iw_comment(iw, "Channels are numbered 1,2,3,4");
    iw_cmt_newline(iw);

    iw_comment(iw, "Group 1: A0,A1,A2,A3");
    iw_entry(iw, "g1_cap", cfg_pinmask_encode(priv->cfg.group_scaps[0], unit_tmp512, true));
    iw_entry(iw, "g1_ch", cfg_pinmask_encode(priv->cfg.group_channels[0], unit_tmp512, true));

    iw_comment(iw, "Group 2: A4,A5,A6,A7");
    iw_entry(iw, "g2_cap", cfg_pinmask_encode(priv->cfg.group_scaps[1], unit_tmp512, true));
    iw_entry(iw, "g2_ch", cfg_pinmask_encode(priv->cfg.group_channels[1], unit_tmp512, true));

    iw_comment(iw, "Group 3: C5,B0,B1,B2");
    iw_entry(iw, "g3_cap", cfg_pinmask_encode(priv->cfg.group_scaps[2], unit_tmp512, true));
    iw_entry(iw, "g3_ch", cfg_pinmask_encode(priv->cfg.group_channels[2], unit_tmp512, true));

    iw_comment(iw, "Group 4: A9,A10,A11,A12");
    iw_entry(iw, "g4_cap", cfg_pinmask_encode(priv->cfg.group_scaps[3], unit_tmp512, true));
    iw_entry(iw, "g4_ch", cfg_pinmask_encode(priv->cfg.group_channels[3], unit_tmp512, true));

    iw_comment(iw, "Group 5: B3,B4,B6,B7");
    iw_entry(iw, "g5_cap", cfg_pinmask_encode(priv->cfg.group_scaps[4], unit_tmp512, true));
    iw_entry(iw, "g5_ch", cfg_pinmask_encode(priv->cfg.group_channels[4], unit_tmp512, true));

    iw_comment(iw, "Group 6: B11,B12,B13,B14");
    iw_entry(iw, "g6_cap", cfg_pinmask_encode(priv->cfg.group_scaps[5], unit_tmp512, true));
    iw_entry(iw, "g6_ch", cfg_pinmask_encode(priv->cfg.group_channels[5], unit_tmp512, true));

#if HAVE_CH7
    iw_comment(iw, "E2,E3,E4,E5");
    iw_entry(iw, "g7_cap", cfg_pinmask_encode(priv->cfg.group_scaps[6], unit_tmp512, true));
    iw_entry(iw, "g7_ch", cfg_pinmask_encode(priv->cfg.group_channels[6], unit_tmp512, true));
#endif
#if HAVE_CH8
    iw_comment(iw, "D12,D13,D14,D15");
    iw_entry(iw, "g8_cap", cfg_pinmask_encode(priv->cfg.group_scaps[7], unit_tmp512, true));
    iw_entry(iw, "g8_ch", cfg_pinmask_encode(priv->cfg.group_channels[7], unit_tmp512, true));
#endif
}

