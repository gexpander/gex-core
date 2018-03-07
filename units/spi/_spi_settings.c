//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define SPI_INTERNAL
#include "_spi_internal.h"

/** Load from a binary buffer stored in Flash */
void USPI_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->periph_num = pp_u8(pp);
    priv->prescaller = pp_u16(pp);
    priv->remap = pp_u8(pp);

    priv->cpol = pp_bool(pp);
    priv->cpha = pp_bool(pp);
    priv->tx_only = pp_bool(pp);
    priv->lsb_first = pp_bool(pp);

    priv->ssn_port_name = pp_char(pp);
    priv->ssn_pins = pp_u16(pp);
}

/** Write to a binary buffer for storing in Flash */
void USPI_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_u8(pb, priv->periph_num);
    pb_u16(pb, priv->prescaller);
    pb_u8(pb, priv->remap);

    pb_bool(pb, priv->cpol);
    pb_bool(pb, priv->cpha);
    pb_bool(pb, priv->tx_only);
    pb_bool(pb, priv->lsb_first);

    pb_char(pb, priv->ssn_port_name);
    pb_u16(pb, priv->ssn_pins);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t USPI_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "device")) {
        priv->periph_num = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "remap")) {
        priv->remap = cfg_u8_parse(value, &suc);
    }
    else if (streq(key, "prescaller")) {
        priv->prescaller = cfg_u16_parse(value, &suc);
    }
    else if (streq(key, "cpol")) {
        priv->cpol = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "cpha")) {
        priv->cpha = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "tx-only")) {
        priv->tx_only = cfg_bool_parse(value, &suc);
    }
    else if (streq(key, "first-bit")) {
        priv->lsb_first = (bool) cfg_enum2_parse(value, "MSB", 0, "LSB", 1, &suc);
    }
    else if (streq(key, "port")) {
        suc = cfg_port_parse(value, &priv->ssn_port_name);
    }
    else if (streq(key, "pins")) {
        priv->ssn_pins = cfg_pinmask_parse(value, &suc);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void USPI_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Peripheral number (SPIx)");
    iw_entry_d(iw, "device", priv->periph_num);

    // TODO show a legend for peripherals and remaps
    iw_comment(iw, "Pin mappings (SCK,MISO,MOSI)");
#if STM32F072xB
    iw_comment(iw, " SPI1: (0) A5,A6,A7     (1) B3,B4,B5"); //  (2) E13,E14,E15
    iw_comment(iw, " SPI2: (0) B13,B14,B15"); //  (1) D1,D3,D4
#elif GEX_PLAT_F103_BLUEPILL
    #error "NO IMPL"
#elif GEX_PLAT_F303_DISCOVERY
    #error "NO IMPL"
#elif GEX_PLAT_F407_DISCOVERY
    #error "NO IMPL"
#else
    #error "BAD PLATFORM!"
#endif
    iw_entry_d(iw, "remap", priv->remap);

    iw_cmt_newline(iw);
    iw_comment(iw, "Prescaller: 2,4,8,...,256");
    iw_entry_d(iw, "prescaller", priv->prescaller);

    iw_comment(iw, "Clock polarity: 0,1 (clock idle level)");
    iw_entry_d(iw, "cpol", priv->cpol);

    iw_comment(iw, "Clock phase: 0,1 (active edge, 0-first, 1-second)");
    iw_entry_d(iw, "cpha", priv->cpha);

    iw_comment(iw, "Transmit only, disable MISO");
    iw_entry_s(iw, "tx-only", str_yn(priv->tx_only));

    iw_comment(iw, "Bit order (LSB or MSB first)");
    iw_entry_s(iw, "first-bit", cfg_enum2_encode((uint32_t) priv->lsb_first, 0, "MSB", 1, "LSB"));

    iw_cmt_newline(iw);
    iw_comment(iw, "SS port name");
    iw_entry(iw, "port", "%c", priv->ssn_port_name);

    iw_comment(iw, "SS pins (comma separated, supports ranges)");
    iw_entry_s(iw, "pins", cfg_pinmask_encode(priv->ssn_pins, unit_tmp512, 0));
}
