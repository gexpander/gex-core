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
}

/** Write to a binary buffer for storing in Flash */
void UADC_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_u16(pb, priv->channels);
    pb_bool(pb, priv->enable_tsense);
    pb_bool(pb, priv->enable_vref);
    pb_u8(pb, priv->sample_time);
    pb_u32(pb, priv->frequency);
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
    iw_comment(iw, "0-7 = A0-A7, 8-9 = B0-B1, 10-15 = C0-C5");
    iw_entry(iw, "channels", "%s", pinmask2str_up(priv->channels, unit_tmp512));

    iw_comment(iw, "Enable Tsense channel");
    iw_entry(iw, "enable_tsense", str_yn(priv->enable_tsense));

    iw_comment(iw, "Enable Vref channel");
    iw_entry(iw, "enable_vref", str_yn(priv->enable_tsense));

    iw_comment(iw, "Sampling time (0-7)");
    iw_entry(iw, "sample_time", "%d", (int)priv->sample_time);

    iw_comment(iw, "Sampling frequency (Hz)");
    iw_entry(iw, "frequency", "%d", (int)priv->frequency);
}

