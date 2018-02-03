//
// Created by MightyPork on 2018/01/14.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_usart.h"

#define UUSART_INTERNAL
#include "_usart_internal.h"

/** Load from a binary buffer stored in Flash */
void UUSART_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->periph_num = pp_u8(pp);
    priv->remap      = pp_u8(pp);

    priv->baudrate   = pp_u32(pp);
    priv->parity     = pp_u8(pp);
    priv->stopbits   = pp_u8(pp);
    priv->direction  = pp_u8(pp);

    priv->hw_flow_control = pp_u8(pp);
    priv->clock_output    = pp_bool(pp);
    priv->cpol            = pp_bool(pp);
    priv->cpha            = pp_bool(pp);
    priv->lsb_first       = pp_bool(pp);
    priv->width           = pp_u8(pp);

    priv->data_inv = pp_bool(pp);
    priv->rx_inv = pp_bool(pp);
    priv->tx_inv = pp_bool(pp);

    priv->de_output = pp_bool(pp);
    priv->de_polarity = pp_bool(pp);
    priv->de_assert_time = pp_u8(pp);
    priv->de_clear_time = pp_u8(pp);
}

/** Write to a binary buffer for storing in Flash */
void UUSART_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_u8(pb, priv->periph_num);
    pb_u8(pb, priv->remap);

    pb_u32(pb, priv->baudrate);
    pb_u8(pb, priv->parity);
    pb_u8(pb, priv->stopbits);
    pb_u8(pb, priv->direction);

    pb_u8(pb, priv->hw_flow_control);
    pb_bool(pb, priv->clock_output);
    pb_bool(pb, priv->cpol);
    pb_bool(pb, priv->cpha);
    pb_bool(pb, priv->lsb_first);
    pb_u8(pb, priv->width);

    pb_bool(pb, priv->data_inv);
    pb_bool(pb, priv->rx_inv);
    pb_bool(pb, priv->tx_inv);

    pb_bool(pb, priv->de_output);
    pb_bool(pb, priv->de_polarity);
    pb_u8(pb, priv->de_assert_time);
    pb_u8(pb, priv->de_clear_time);
}

/** Parse a key-value pair from the INI file */
error_t UUSART_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "device")) {
        priv->periph_num = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "remap")) {
        priv->remap = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "baud-rate")) {
        priv->baudrate = (uint32_t ) avr_atoi(value);
    }
    else if (streq(key, "parity")) {
        priv->parity = (uint8_t) str_parse_3(value,
                                             "NONE", 0,
                                             "ODD", 1,
                                             "EVEN", 2, &suc);
    }
    else if (streq(key, "stop-bits")) {
        priv->stopbits = (uint8_t) str_parse_4(value,
                                               "0.5", 0,
                                               "1", 1,
                                               "1.5", 2,
                                               "2", 3, &suc);
    }
    else if (streq(key, "direction")) {
        priv->direction = (uint8_t) str_parse_3(value,
                                                "RX", 1,
                                                "TX", 2,
                                                "RXTX", 3, &suc);
    }
    else if (streq(key, "hw-flow-control")) {
        priv->hw_flow_control = (uint8_t) str_parse_4(value,
                                                      "NONE", 0,
                                                      "RTS", 1,
                                                      "CTS", 2,
                                                      "FULL", 3, &suc);
    }
    else if (streq(key, "word-width")) {
        priv->width = (uint8_t ) avr_atoi(value);
    }
    else if (streq(key, "first-bit")) {
        priv->lsb_first = (bool)str_parse_2(value, "MSB", 0, "LSB", 1, &suc);
    }
    else if (streq(key, "clock-output")) {
        priv->clock_output = str_parse_yn(value, &suc);
    }
    else if (streq(key, "cpol")) {
        priv->cpol = (bool) avr_atoi(value);
    }
    else if (streq(key, "cpha")) {
        priv->cpha = (bool) avr_atoi(value);
    }
    else if (streq(key, "de-output")) {
        priv->de_output = str_parse_yn(value, &suc);
    }
    else if (streq(key, "de-polarity")) {
        priv->de_polarity = (bool) avr_atoi(value);
    }
    else if (streq(key, "de-assert-time")) {
        priv->de_assert_time = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "de-clear-time")) {
        priv->de_clear_time = (uint8_t) avr_atoi(value);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
void UUSART_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Peripheral number (UARTx 1-4)");
    iw_entry(iw, "device", "%d", (int)priv->periph_num);

    iw_comment(iw, "Pin mappings (TX,RX,CK,CTS,RTS/DE)");
#if GEX_PLAT_F072_DISCOVERY
    iw_comment(iw, " USART1: (0) A9,A10,A8,A11,A12   (1) B6,B7,A8,A11,A12");
    iw_comment(iw, " USART2: (0) A2,A3,A4,A0,A1      (1) A14,A15,A4,A0,A1");
    iw_comment(iw, " USART3: (0) B10,B11,B12,B13,B14");
    iw_comment(iw, " USART4: (0) A0,A1,C12,B7,A15    (1) C10,C11,C12,B7,A15");
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
    iw_comment(iw, "Baud rate in bps (eg. 9600, 115200)"); // TODO examples/range
    iw_entry(iw, "baud-rate", "%d", (int)priv->baudrate);

    iw_comment(iw, "Parity type (NONE, ODD, EVEN)");
    iw_entry(iw, "parity", "%s", str_3(priv->parity,
                                       0, "NONE",
                                       1, "ODD",
                                       2, "EVEN"));

    iw_comment(iw, "Number of stop bits (0.5, 1, 1.5, 2)");
    iw_entry(iw, "stop-bits", "%s", str_4(priv->stopbits,
                                          0, "0.5",
                                          1, "1",
                                          2, "1.5",
                                          3, "2"));

    iw_comment(iw, "Bit order (LSB or MSB first)");
    iw_entry(iw, "first-bit", str_2((uint32_t)priv->lsb_first,
                                    0, "MSB",
                                    1, "LSB"));

    iw_comment(iw, "Word width (7,8,9) - including parity bit if used");
    iw_entry(iw, "word-width", "%d", (int)priv->width);

    iw_comment(iw, "Enabled lines (RX,TX,RXTX)");
    iw_entry(iw, "direction", str_3(priv->direction,
                                    1, "RX",
                                    2, "TX",
                                    3, "RXTX"));

    iw_comment(iw, "Hardware flow control (NONE, RTS, CTS, FULL)");
    iw_entry(iw, "hw-flow-control", "%s", str_4(priv->hw_flow_control,
                                                0, "NONE",
                                                1, "RTS",
                                                2, "CTS",
                                                3, "FULL"));

    iw_cmt_newline(iw);
    iw_comment(iw, "Generate serial clock (Y,N)");
    iw_entry(iw, "clock-output", str_yn(priv->clock_output));
    iw_comment(iw, "Output clock polarity: 0,1 (clock idle level)");
    iw_entry(iw, "cpol", "%d", (int)priv->cpol);
    iw_comment(iw, "Output clock phase: 0,1 (active edge, 0-first, 1-second)");
    iw_entry(iw, "cpha", "%d", (int)priv->cpha);

    iw_cmt_newline(iw);
    iw_comment(iw, "Generate RS485 Driver Enable signal (Y,N) - uses RTS pin");
    iw_entry(iw, "de-output", str_yn(priv->de_output));
    iw_comment(iw, "DE active level: 0,1");
    iw_entry(iw, "de-polarity", "%d", (int)(priv->de_polarity));
    iw_comment(iw, "DE assert time (0-31)");
    iw_entry(iw, "de-assert-time", "%d", (int)(priv->de_assert_time));
    iw_comment(iw, "DE clear time (0-31)");
    iw_entry(iw, "de-clear-time", "%d", (int)(priv->de_clear_time));
}
