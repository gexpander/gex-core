//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define SPI_INTERNAL
#include "_spi_internal.h"
#include "_spi_settings.h"

/** Load from a binary buffer stored in Flash */
void SPI_loadBinary(Unit *unit, PayloadParser *pp)
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
void SPI_writeBinary(Unit *unit, PayloadBuilder *pb)
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
error_t SPI_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "device")) {
        priv->periph_num = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "remap")) {
        priv->remap = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "prescaller")) {
        priv->prescaller = (uint16_t ) avr_atoi(value);
    }
    else if (streq(key, "cpol")) {
        priv->cpol = (bool) avr_atoi(value);
    }
    else if (streq(key, "cpha")) {
        priv->cpha = (bool) avr_atoi(value);
    }
    else if (streq(key, "tx-only")) {
        priv->tx_only = str_parse_yn(value, &suc);
    }
    else if (streq(key, "first-bit")) {
        priv->lsb_first = (bool)str_parse_2(value, "MSB", 0, "LSB", 1, &suc);
    }
    else if (streq(key, "port")) {
        suc = parse_port_name(value, &priv->ssn_port_name);
    }
    else if (streq(key, "pins")) {
        priv->ssn_pins = parse_pinmask(value, &suc);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void SPI_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Peripheral number (SPIx)");
    iw_entry(iw, "device", "%d", (int)priv->periph_num);

    // TODO show a legend for peripherals and remaps
    iw_comment(iw, "Pin mappings (SCK,MISO,MOSI)");
#if GEX_PLAT_F072_DISCOVERY
    iw_comment(iw, " SPI1: (0) A5,A6,A7     (1) B3,B4,B5  (2) E13,E14,E15");
    iw_comment(iw, " SPI2: (0) B13,B14,B15  (1) D1,D3,D4");
#elif GEX_PLAT_F103_BLUEPILL
    #error "NO IMPL"
#elif GEX_PLAT_F303_DISCOVERY
    #error "NO IMPL"
#elif GEX_PLAT_F407_DISCOVERY
    #error "NO IMPL"
#else
    #error "BAD PLATFORM!"
#endif
    iw_entry(iw, "remap", "%d", (int)priv->remap);

    iw_cmt_newline(iw);
    iw_comment(iw, "Prescaller: 2,4,8,...,256");
    iw_entry(iw, "prescaller", "%d", (int)priv->prescaller);

    iw_comment(iw, "Clock polarity: 0,1 (clock idle level)");
    iw_entry(iw, "cpol", "%d", (int)priv->cpol);

    iw_comment(iw, "Clock phase: 0,1 (active edge, 0-first, 1-second)");
    iw_entry(iw, "cpha", "%d", (int)priv->cpha);

    iw_comment(iw, "Transmit only, disable MISO");
    iw_entry(iw, "tx-only", str_yn(priv->tx_only));

    iw_comment(iw, "Bit order (LSB or MSB first)");
    iw_entry(iw, "first-bit", str_2((uint32_t)priv->lsb_first,
                                    0, "MSB",
                                    1, "LSB"));

    iw_cmt_newline(iw);
    iw_comment(iw, "SS port name");
    iw_entry(iw, "port", "%c", priv->ssn_port_name);

    iw_comment(iw, "SS pins (comma separated, supports ranges)");
    iw_entry(iw, "pins", "%s", pinmask2str(priv->ssn_pins, unit_tmp512));
}
