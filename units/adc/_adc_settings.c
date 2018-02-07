//
// Created by MightyPork on 2018/02/03.
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

    priv->channels = pp_u16(pp);
    priv->enable_tsense = pp_bool(pp);
    priv->enable_vref = pp_bool(pp);
    priv->sample_time = pp_u8(pp);
    priv->frequency = pp_u32(pp);

    if (version >= 1) {
        priv->buffer_size = pp_u16(pp);
    }
    if (version >= 2) {
        priv->averaging_factor = pp_u16(pp);
    }
}

/** Write to a binary buffer for storing in Flash */
void UADC_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 2); // version

    pb_u16(pb, priv->channels);
    pb_bool(pb, priv->enable_tsense);
    pb_bool(pb, priv->enable_vref);
    pb_u8(pb, priv->sample_time);
    pb_u32(pb, priv->frequency);
    pb_u16(pb, priv->buffer_size);
    pb_u16(pb, priv->averaging_factor);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UADC_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "channels")) {
        priv->channels = parse_pinmask(value, &suc);
    }
    else if (streq(key, "enable_tsense")) {
        priv->enable_tsense = str_parse_yn(value, &suc);
    }
    else if (streq(key, "enable_vref")) {
        priv->enable_vref = str_parse_yn(value, &suc);
    }
    else if (streq(key, "sample_time")) {
        priv->sample_time = (uint8_t) avr_atoi(value);
        if (priv->sample_time > 7) return E_BAD_VALUE;
    }
    else if (streq(key, "frequency")) {
        priv->frequency = (uint32_t) avr_atoi(value);
    }
    else if (streq(key, "buffer_size")) {
        priv->buffer_size = (uint16_t) avr_atoi(value);
    }
    else if (streq(key, "avg_factor")) {
        priv->averaging_factor = (uint16_t) avr_atoi(value);
        if (priv->averaging_factor > 1000) return E_BAD_VALUE;
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
    iw_comment(iw, "0-7 = A0-A7,  8-9 = B0-B1,  10-15 = C0-C5");
    iw_entry(iw, "channels", "%s", pinmask2str_up(priv->channels, unit_tmp512));

    iw_comment(iw, "Enable Tsense channel (#16)");
    iw_entry(iw, "enable_tsense", str_yn(priv->enable_tsense));

    iw_comment(iw, "Enable Vref channel (#17)");
    iw_entry(iw, "enable_vref", str_yn(priv->enable_vref));

    iw_cmt_newline(iw);
    iw_comment(iw, "Sampling time (0-7)");
    iw_entry(iw, "sample_time", "%d", (int)priv->sample_time);

    iw_comment(iw, "Sampling frequency (Hz)");
    iw_entry(iw, "frequency", "%d", (int)priv->frequency);

    iw_comment(iw, "Sample buffer size (bytes, 2 per channels per sample)");
    iw_comment(iw, "- a report is sent when 1/2 of the circular buffer is filled");
    iw_comment(iw, "- the buffer is shared by all channels");
    iw_comment(iw, "- insufficient buffer size can lead to data loss");
    iw_entry(iw, "buffer_size", "%d", (int)priv->buffer_size);

    iw_cmt_newline(iw);
    iw_comment(iw, "Exponential averaging coefficient (permil, range 0-1000 ~ 0.000-1.000)");
    iw_comment(iw, "- used formula: y[t]=(1-k)*y[t-1]+k*u[t]");
    iw_comment(iw, "- available only for direct readout (i.e. not used in block capture)");
    iw_entry(iw, "avg_factor", "%d", priv->averaging_factor);
}

