//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define DAC_INTERNAL
#include "_dac_internal.h"

/** Load from a binary buffer stored in Flash */
void UDAC_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->cfg.ch[0].enable = pp_bool(pp);
    priv->cfg.ch[0].buffered = pp_bool(pp);
    priv->cfg.ch[0].noise_type = (enum UDAC_Noise) pp_u8(pp);
    priv->cfg.ch[0].noise_level = pp_u8(pp);

    priv->cfg.ch[1].enable = pp_bool(pp);
    priv->cfg.ch[1].buffered = pp_bool(pp);
    priv->cfg.ch[1].noise_type = (enum UDAC_Noise) pp_u8(pp);
    priv->cfg.ch[1].noise_level = pp_u8(pp);
}

/** Write to a binary buffer for storing in Flash */
void UDAC_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_bool(pb, priv->cfg.ch[0].enable);
    pb_bool(pb, priv->cfg.ch[0].buffered);
    pb_u8(pb, priv->cfg.ch[0].noise_type);
    pb_u8(pb, priv->cfg.ch[0].noise_level);

    pb_bool(pb, priv->cfg.ch[1].enable);
    pb_bool(pb, priv->cfg.ch[1].buffered);
    pb_u8(pb, priv->cfg.ch[1].noise_type);
    pb_u8(pb, priv->cfg.ch[1].noise_level);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UDAC_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    // Ch1
    if (streq(key, "ch1_enable")) {
        priv->cfg.ch[0].enable = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "ch1_buff")) {
        priv->cfg.ch[0].buffered = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "ch1_noise")) {
        priv->cfg.ch[0].noise_type =
            (enum UDAC_Noise) cfg_enum3_parse(value,
                                             "NONE", NOISE_NONE,
                                             "WHITE", NOISE_WHITE,
                                             "TRIANGLE", NOISE_TRIANGLE, &suc);
    }
    else if (streq(key, "ch1_noise-level")) {
        uint8_t x = cfg_u8_parse(value, &suc);
        if (x == 0) x = 1;
        if (x > 12) x = 12;
        priv->cfg.ch[0].noise_level = (uint8_t) (x - 1);
    }
    // Ch2
    else if (streq(key, "ch2_enable")) {
        priv->cfg.ch[1].enable = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "ch2_buff")) {
        priv->cfg.ch[1].buffered = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "ch2_noise")) {
        priv->cfg.ch[1].noise_type =
            (enum UDAC_Noise) cfg_enum3_parse(value,
                                              "NONE", NOISE_NONE,
                                              "WHITE", NOISE_WHITE,
                                              "TRIANGLE", NOISE_TRIANGLE, &suc);
    }
    else if (streq(key, "ch2_noise-level")) {
        uint8_t x = cfg_u8_parse(value, &suc);
        if (x == 0) x = 1;
        if (x > 12) x = 12;
        priv->cfg.ch[1].noise_level = (uint8_t) (x - 1);
    }
    // end
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void UDAC_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Enabled channels (1:A4, 2:A5)");
    iw_entry_s(iw, "ch1_enable", str_yn(priv->cfg.ch[0].enable));
    iw_entry_s(iw, "ch2_enable", str_yn(priv->cfg.ch[1].enable));

    iw_comment(iw, "Enable output buffer");
    iw_entry_s(iw, "ch1_buff", str_yn(priv->cfg.ch[0].buffered));
    iw_entry_s(iw, "ch2_buff", str_yn(priv->cfg.ch[1].buffered));

    iw_comment(iw, "Superimposed noise type (NONE,WHITE,TRIANGLE) and nbr. of bits (1-12)");

    iw_entry_s(iw, "ch1_noise", cfg_enum3_encode(priv->cfg.ch[0].noise_type,
                                                 NOISE_NONE, "NONE",
                                                 NOISE_WHITE, "WHITE",
                                                 NOISE_TRIANGLE, "TRIANGLE"));
    iw_entry_d(iw, "ch1_noise-level", priv->cfg.ch[0].noise_level + 1);

    iw_entry_s(iw, "ch2_noise", cfg_enum3_encode(priv->cfg.ch[1].noise_type,
                                                 NOISE_NONE, "NONE",
                                                 NOISE_WHITE, "WHITE",
                                                 NOISE_TRIANGLE, "TRIANGLE"));
    iw_entry_d(iw, "ch2_noise-level", priv->cfg.ch[1].noise_level + 1);
}
