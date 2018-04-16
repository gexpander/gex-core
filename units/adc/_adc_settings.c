//
// Created by MightyPork on 2018/02/03.
//
// ADC unit settings reading / parsing
//

#include "platform.h"
#include "unit_base.h"

#define ADC_INTERNAL
#include "_adc_internal.h"

/** Load from a binary buffer stored in Flash */
void UADC_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->cfg.channels = pp_u32(pp);
    priv->cfg.sample_time = pp_u8(pp);
    priv->cfg.frequency = pp_u32(pp);
    priv->cfg.buffer_size = pp_u32(pp);
    priv->cfg.averaging_factor = pp_u16(pp);

    if (version >= 1) {
        priv->cfg.enable_averaging = pp_bool(pp);
    }
}

/** Write to a binary buffer for storing in Flash */
void UADC_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 1); // version

    pb_u32(pb, priv->cfg.channels);
    pb_u8(pb, priv->cfg.sample_time);
    pb_u32(pb, priv->cfg.frequency);
    pb_u32(pb, priv->cfg.buffer_size);
    pb_u16(pb, priv->cfg.averaging_factor);
    pb_bool(pb, priv->cfg.enable_averaging);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UADC_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "channels")) {
        priv->cfg.channels = cfg_pinmask_parse_32(value, &suc);
    }
    else if (streq(key, "sample_time")) {
        priv->cfg.sample_time = cfg_u8_parse(value, &suc);
        if (priv->cfg.sample_time > 7) return E_BAD_VALUE;
    }
    else if (streq(key, "frequency")) {
        priv->cfg.frequency = cfg_u32_parse(value, &suc);
    }
    else if (streq(key, "buffer_size")) {
        priv->cfg.buffer_size = cfg_u32_parse(value, &suc);
    }
    else if (streq(key, "avg_factor")) {
        priv->cfg.averaging_factor = cfg_u16_parse(value, &suc);
        if (priv->cfg.averaging_factor > 1000) return E_BAD_VALUE;
    }
    else if (streq(key, "averaging")) {
        priv->cfg.enable_averaging = cfg_bool_parse(value, &suc);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void UADC_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Enabled channels, comma separated");
    iw_comment(iw, " 0  1  2  3  4  5  6  7    8  9   10 11 12 13 14 15   16    17");
    iw_comment(iw, "A0 A1 A2 A3 A4 A5 A6 A7   B0 B1   C0 C1 C2 C3 C4 C5   Tsens Vref");
    iw_entry_s(iw, "channels", cfg_pinmask_encode(priv->cfg.channels, unit_tmp512, true));

    iw_cmt_newline(iw);
    iw_comment(iw, "Sampling time (0-7)");
    iw_entry_d(iw, "sample_time", priv->cfg.sample_time);

    iw_comment(iw, "Sampling frequency (Hz)");
    iw_entry_d(iw, "frequency", priv->cfg.frequency);

    iw_cmt_newline(iw);
    iw_comment(iw, "Sample buffer size");
    iw_comment(iw, "- shared by all enabled channels");
    iw_comment(iw, "- defines the maximum pre-trigger size (divide by # of channels)");
    iw_comment(iw, "- captured data is sent in half-buffer chunks");
    iw_comment(iw, "- buffer overrun aborts the data capture");
    iw_entry_d(iw, "buffer_size", priv->cfg.buffer_size);

    iw_cmt_newline(iw);
    iw_comment(iw, "Enable continuous sampling with averaging");
    iw_comment(iw, "Caution: This can cause DAC output glitches");
    iw_entry_s(iw, "averaging", str_yn(priv->cfg.enable_averaging));
    iw_comment(iw, "Exponential averaging coefficient (permil, range 0-1000 ~ 0.000-1.000)");
    iw_comment(iw, "- used formula: y[t]=(1-k)*y[t-1]+k*u[t]");
    iw_comment(iw, "- not available when a capture is running");
    iw_entry_d(iw, "avg_factor", priv->cfg.averaging_factor);
}

