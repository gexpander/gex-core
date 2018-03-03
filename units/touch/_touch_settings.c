//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define TOUCH_INTERNAL
#include "_touch_internal.h"

const char *utouch_group_labels[8] = {
    "1:A0, 2:A1, 3:A2, 4:A3",
    "1:A4, 2:A5, 3:A6, 4:A7",
    "1:C5, 2:B0, 3:B1, 4:B2",
    "1:A9, 2:A10, 3:A11, 4:A12",
    "1:B3, 2:B4, 3:B6, 4:B7",
    "1:B11, 2:B12, 3:B13, 4:B14",
    "1:E2, 2:E3, 3:E4, 4:E5",
    "1:D12, 2:D13, 3:D14, 4:D15",
};

const Resource utouch_group_rscs[8][4] = {
    {R_PA0, R_PA1, R_PA2, R_PA3},
    {R_PA4, R_PA5, R_PA6, R_PA7},
    {R_PC5, R_PB0, R_PB1, R_PB2},
    {R_PA9, R_PA10, R_PA11, R_PA12},
    {R_PB3, R_PB4, R_PB6, R_PB7},
    {R_PB11, R_PB12, R_PB13, R_PB14},
    {R_PE2, R_PE3, R_PE4, R_PE5},
    {R_PD12, R_PD13, R_PD14, R_PD15},
};

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

    if (version >= 1) {
        priv->cfg.interlaced = pp_bool(pp);
    }

    if (version >= 2) {
        priv->cfg.binary_debounce_ms = pp_u16(pp);
        priv->cfg.binary_hysteresis = pp_u16(pp);
    }
}

/** Write to a binary buffer for storing in Flash */
void UTOUCH_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 2); // version

    pb_u8(pb, priv->cfg.charge_time);
    pb_u8(pb, priv->cfg.drain_time);
    pb_u8(pb, priv->cfg.spread_deviation);
    pb_u8(pb, priv->cfg.ss_presc);
    pb_u8(pb, priv->cfg.pg_presc);
    pb_u8(pb, priv->cfg.sense_timeout);
    pb_buf(pb, priv->cfg.group_scaps, 8);
    pb_buf(pb, priv->cfg.group_channels, 8);
    pb_bool(pb, priv->cfg.interlaced);
    pb_u16(pb, priv->cfg.binary_debounce_ms);
    pb_u16(pb, priv->cfg.binary_hysteresis);
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
    else if (streq(key, "interlaced-pads")) {
        priv->cfg.interlaced = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "btn-debounce")) {
        priv->cfg.binary_debounce_ms = cfg_u16_parse(value, &suc);
    }
    else if (streq(key, "btn-hysteresis")) {
        priv->cfg.binary_hysteresis = cfg_u16_parse(value, &suc);
    }
    else {
        volatile char namebuf[10]; // must be volatile or gcc optimizes out the second compare and fucks it up

        for (int i = 0; i < 6; i++) { // skip 7,8
            SPRINTF(namebuf, "g%d_cap", i+1);
            if (streq(key, namebuf)) {
                priv->cfg.group_scaps[i] = (uint8_t) cfg_pinmask_parse(value, &suc);
                goto matched;
            }

            SPRINTF(namebuf, "g%d_ch", i+1);
            if (streq(key, namebuf)) {
                priv->cfg.group_channels[i] = (uint8_t) cfg_pinmask_parse(value, &suc);
                goto matched;
            }
        }

        return E_BAD_KEY;
    }

matched:
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
    iw_entry_d(iw, "pg-clock-prediv", priv->cfg.pg_presc);
    iw_comment(iw, "Sense pad charging time (1-16)");
    iw_entry_d(iw, "charge-time", priv->cfg.charge_time);
    iw_comment(iw, "Charge transfer time (1-16)");
    iw_entry_d(iw, "drain-time", priv->cfg.drain_time);
    iw_comment(iw, "Measurement timeout (1-7)");
    iw_entry_d(iw, "sense-timeout", priv->cfg.sense_timeout);

    iw_cmt_newline(iw);
    iw_comment(iw, "Spread spectrum max deviation (0-128,0=off)");
    iw_entry_d(iw, "ss-deviation", priv->cfg.spread_deviation);
    iw_comment(iw, "Spreading clock prescaller (1,2)");
    iw_entry_d(iw, "ss-clock-prediv", priv->cfg.ss_presc);

    iw_cmt_newline(iw);
    iw_comment(iw, "Optimize for interlaced pads (individual sampling with others floating)");
    iw_entry_s(iw, "interlaced-pads", str_yn(priv->cfg.interlaced));

    iw_cmt_newline(iw);
    iw_comment(iw, "Button mode debounce (ms) and release hysteresis (lsb)");
    iw_entry_d(iw, "btn-debounce", priv->cfg.binary_debounce_ms);
    iw_entry_d(iw, "btn-hysteresis", priv->cfg.binary_hysteresis);

    iw_cmt_newline(iw);
    iw_comment(iw, "Each used group must have 1 sampling capacitor and 1-3 channels.");
    iw_comment(iw, "Channels are numbered 1,2,3,4");
    iw_cmt_newline(iw);

    char namebuf[10];
    for (int i = 0; i < 6; i++) { // skip 7,8
        iw_commentf(iw,  "Group%d - %s", i+1, utouch_group_labels[i]);
        SPRINTF(namebuf, "g%d_cap", i+1);
        iw_entry_s(iw, namebuf, cfg_pinmask_encode(priv->cfg.group_scaps[i], unit_tmp512, true));
        SPRINTF(namebuf, "g%d_ch", i+1);
        iw_entry_s(iw, namebuf, cfg_pinmask_encode(priv->cfg.group_channels[i], unit_tmp512, true));
    }
}

