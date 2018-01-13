//
// Created by MightyPork on 2018/01/02.
//

#include <stm32f072xb.h>
#include "platform.h"
#include "comm/messages.h"
#include "unit_base.h"
#include "utils/avrlibc.h"
#include "unit_uart.h"

// SPI master

/** Private data structure */
struct priv {
    uint8_t periph_num; //!< 1-6
    uint8_t remap;      //!< UART remap option

    uint32_t baudrate;  //!< baud rate
    uint8_t parity;     //!< 0-none, 1-odd, 2-even
    uint8_t stopbits;   //!< 0-half, 1-one, 2-one-and-half, 3-two (halves - 1)
    uint8_t direction;  //!< 1-RX, 2-TX, 3-RXTX

    uint8_t hw_flow_control; //!< HW flow control 0-none, 1-RTC, 2-CTS, 3-full
    bool clock_output;  //!< Output serial clock
    bool cpol;          //!< clock CPOL setting
    bool cpha;          //!< clock CPHA setting
    bool lsb_first;     //!< bit order
    uint8_t width;      //!< word width - 7, 8, 9 (this includes parity)

    bool data_inv;  //!< Invert data bytes
    bool rx_inv;    //!< Invert the RX pin levels
    bool tx_inv;    //!< Invert the TX pin levels

    bool de_output; //!< Generate the Driver Enable signal for RS485
    bool de_polarity; //!< DE active level
    uint8_t de_assert_time;   //!< Time to assert the DE signal before transmit
    uint8_t de_clear_time; //!< Time to clear the DE signal after transmit

    USART_TypeDef *periph;
};

/** Allocate data structure and set defaults */
static error_t UUSART_preInit(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv), &suc);
    if (!suc) return E_OUT_OF_MEM;

    // some defaults
    priv->periph_num = 1;
    priv->remap = 0;

    priv->baudrate = 115200;
    priv->parity = 0;         //!< 0-none, 1-odd, 2-even
    priv->stopbits = 1;       //!< 0-half, 1-one, 2-1.5, 3-two
    priv->direction = 3; // RXTX

    priv->hw_flow_control = false;
    priv->clock_output = false;
    priv->cpol = 0;
    priv->cpha = 0;
    priv->lsb_first = true; // LSB first is default for UART
    priv->width = 8;

    priv->data_inv = false;
    priv->rx_inv = false;
    priv->tx_inv = false;

    priv->de_output = false;
    priv->de_polarity = 1; // active high
    // this should equal to a half-byte length when oversampling by 16 is used (default)
    priv->de_assert_time = 8;
    priv->de_clear_time = 8;

    return E_SUCCESS;
}

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
static void UUSART_loadBinary(Unit *unit, PayloadParser *pp)
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
static void UUSART_writeBinary(Unit *unit, PayloadBuilder *pb)
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

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
static error_t UUSART_loadIni(Unit *unit, const char *key, const char *value)
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
static void UUSART_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Peripheral number (UARTx 1-4)");
    iw_entry(iw, "device", "%d", (int)priv->periph_num);

    iw_comment(iw, "Pin mappings (TX,RX,CK,CTS,RTS)");
#if GEX_PLAT_F072_DISCOVERY
    // TODO
    iw_comment(iw, " USART1: (0) A9,A10,A8,A11,A12   (1) B6,B7,A8,A11,A12");
    iw_comment(iw, " USART2: (0) A2,A3,A4,A0,A1      (1) D5,D6,D7,D3,D4");
    iw_comment(iw, " USART3: (0) B10,B11,B12,B13,B14 (1) D8,D9,D10,D11,D12");
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

// ------------------------------------------------------------------------

struct paf {
    char port;
    uint8_t pin;
    uint8_t af;
};

/** Finalize unit set-up */
static error_t UUSART_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (!(priv->periph_num >= 1 && priv->periph_num <= 4)) {
        dbg("!! Bad USART periph");
        return E_BAD_CONFIG;
    }

    // assign and claim the peripheral
    if (priv->periph_num == 1) {
        TRY(rsc_claim(unit, R_USART1));
        priv->periph = USART1;
    }
#if defined(USART2)
    else if (priv->periph_num == 2) {
        TRY(rsc_claim(unit, R_USART2));
        priv->periph = USART2;
    }
#endif
#if defined(USART3)
    else if (priv->periph_num == 3) {
        TRY(rsc_claim(unit, R_USART3));
        priv->periph = USART3;
    }
#endif
#if defined(USART4)
    else if (priv->periph_num == 4) {
        TRY(rsc_claim(unit, R_USART4));
        priv->periph = USART4;
    }
#endif
    else return E_BAD_CONFIG;


    // This is written for F072, other platforms will need adjustments

    // Configure UART pins (AF)

    const struct paf *mappings = NULL;

    // TODO
#if GEX_PLAT_F072_DISCOVERY
    const struct paf mapping_1_0[5] = {
        {'A', 8, LL_GPIO_AF_1}, // CK
        {'A', 9, LL_GPIO_AF_1}, // TX
        {'A', 10, LL_GPIO_AF_1}, // RX
        {'A', 11, LL_GPIO_AF_1}, // CTS
        {'A', 12, LL_GPIO_AF_1}, // RTS
    };

    const struct paf mapping_1_1[5] = {
        {'A', 8, LL_GPIO_AF_1}, // CK
        {'B', 6, LL_GPIO_AF_1}, // TX
        {'B', 7, LL_GPIO_AF_1}, // RX
        {'A', 11, LL_GPIO_AF_1}, // CTS
        {'A', 12, LL_GPIO_AF_1}, // RTS
    };

    const struct paf mapping_2_0[5] = {
        {'A', 4, LL_GPIO_AF_1}, // CK
        {'A', 2, LL_GPIO_AF_1}, // TX
        {'A', 3, LL_GPIO_AF_1}, // RX
        {'A', 0, LL_GPIO_AF_1}, // CTS
        {'A', 1, LL_GPIO_AF_1}, // RTS
    };

    const struct paf mapping_2_1[5] = {
        {'D', 7, LL_GPIO_AF_0}, // CK
        {'D', 5, LL_GPIO_AF_0}, // TX
        {'D', 6, LL_GPIO_AF_0}, // RX
        {'D', 3, LL_GPIO_AF_0}, // CTS
        {'D', 4, LL_GPIO_AF_0}, // RTS
    };

    const struct paf mapping_3_0[5] = {
        {'B', 12, LL_GPIO_AF_4}, // CK
        {'B', 10, LL_GPIO_AF_4}, // TX
        {'B', 11, LL_GPIO_AF_4}, // RX
        {'B', 13, LL_GPIO_AF_4}, // CTS
        {'B', 14, LL_GPIO_AF_4}, // RTS
    };

    const struct paf mapping_3_1[5] = {
        {'D', 10, LL_GPIO_AF_0}, // CK
        {'D', 8, LL_GPIO_AF_0}, // TX
        {'D', 9, LL_GPIO_AF_0}, // RX
        {'D', 11, LL_GPIO_AF_0}, // CTS
        {'D', 12, LL_GPIO_AF_0}, // RTS
    };

    const struct paf mapping_4_0[5] = {
        {'C', 12, LL_GPIO_AF_0}, // CK
        {'A', 0, LL_GPIO_AF_4}, // TX
        {'A', 1, LL_GPIO_AF_4}, // RX
        {'B', 7, LL_GPIO_AF_4}, // CTS
        {'A', 15, LL_GPIO_AF_4}, // RTS
    };

    const struct paf mapping_4_1[5] = {
        {'C', 12, LL_GPIO_AF_0}, // CK
        {'C', 10, LL_GPIO_AF_0}, // TX
        {'C', 11, LL_GPIO_AF_0}, // RX
        {'B', 7, LL_GPIO_AF_4}, // CTS
        {'A', 15, LL_GPIO_AF_4}, // RTS
    };

    if (priv->periph_num == 1) {
        // USART1
        if (priv->remap == 0) mappings = &mapping_1_0[0];
        else if (priv->remap == 1) mappings = &mapping_1_1[0];
        else return E_BAD_CONFIG;
    }
    else if (priv->periph_num == 2) {
        // USART2
        if (priv->remap == 0) mappings = &mapping_2_0[0];
        else if (priv->remap == 1) mappings = &mapping_2_1[0];
        else return E_BAD_CONFIG;
    }
    else if (priv->periph_num == 3) {
        // USART3
        if (priv->remap == 0) mappings = &mapping_3_0[0];
        else if (priv->remap == 1) mappings = &mapping_3_1[0];
        else return E_BAD_CONFIG;
    }
    else if (priv->periph_num == 4) {
        // USART3
        if (priv->remap == 0) mappings = &mapping_4_0[0];
        else if (priv->remap == 1) mappings = &mapping_4_1[0];
        else return E_BAD_CONFIG;
    }
    else return E_BAD_CONFIG;
    // TODO other periphs

#elif GEX_PLAT_F103_BLUEPILL
    #error "NO IMPL"
#elif GEX_PLAT_F303_DISCOVERY
    #error "NO IMPL"
#elif GEX_PLAT_F407_DISCOVERY
    #error "NO IMPL"
#else
    #error "BAD PLATFORM!"
#endif

    // CK
    if (priv->clock_output) {
        TRY(rsc_claim_pin(unit, mappings[0].port, mappings[0].pin));
        configure_gpio_alternate( mappings[0].port, mappings[0].pin, mappings[0].af);
    }
    // TX
    if (priv->direction & 2) {
        TRY(rsc_claim_pin(unit, mappings[1].port, mappings[1].pin));
        configure_gpio_alternate( mappings[1].port, mappings[1].pin, mappings[1].af);
    }
    // RX
    if (priv->direction & 1) {
        TRY(rsc_claim_pin(unit, mappings[2].port, mappings[2].pin));
        configure_gpio_alternate( mappings[2].port, mappings[2].pin, mappings[2].af);
    }
    // CTS
    if (priv->hw_flow_control==2 || priv->hw_flow_control==3) {
        TRY(rsc_claim_pin(unit, mappings[4].port, mappings[4].pin));
        configure_gpio_alternate( mappings[4].port, mappings[4].pin, mappings[4].af);
    }
    // RTS
    if (priv->de_output || priv->hw_flow_control==1 || priv->hw_flow_control==3) {
        TRY(rsc_claim_pin(unit, mappings[5].port, mappings[5].pin));
        configure_gpio_alternate( mappings[5].port, mappings[5].pin, mappings[5].af);
    }

    LL_USART_Disable(priv->periph);
    {
        LL_USART_DeInit(priv->periph);
        LL_USART_SetBaudRate(priv->periph,
                             PLAT_AHB_MHZ*1000000,//FIXME this isn't great ...
                             LL_USART_OVERSAMPLING_16,
                             priv->baudrate);

        LL_USART_SetParity(priv->periph,
                           priv->parity == 0 ? LL_USART_PARITY_NONE :
                           priv->parity == 1 ? LL_USART_PARITY_ODD
                                             : LL_USART_PARITY_EVEN);

        LL_USART_SetStopBitsLength(priv->periph,
                                   priv->stopbits == 0 ? LL_USART_STOPBITS_0_5 :
                                   priv->stopbits == 1 ? LL_USART_STOPBITS_1 :
                                   priv->stopbits == 2 ? LL_USART_STOPBITS_1_5
                                                       : LL_USART_STOPBITS_2);

        LL_USART_SetTransferDirection(priv->periph,
                                      priv->direction == 1 ? LL_USART_DIRECTION_RX :
                                      priv->direction == 2 ? LL_USART_DIRECTION_TX
                                                           : LL_USART_DIRECTION_TX_RX);

        LL_USART_SetHWFlowCtrl(priv->periph,
                               priv->hw_flow_control == 0 ? LL_USART_HWCONTROL_NONE :
                               priv->hw_flow_control == 1 ? LL_USART_HWCONTROL_RTS :
                               priv->hw_flow_control == 2 ? LL_USART_HWCONTROL_CTS
                                                          : LL_USART_HWCONTROL_RTS_CTS);

        LL_USART_ConfigClock(priv->periph,
                             priv->cpha ? LL_USART_PHASE_2EDGE
                                        : LL_USART_PHASE_1EDGE,
                             priv->cpol ? LL_USART_POLARITY_HIGH
                                        : LL_USART_POLARITY_LOW,
                             true); // clock on last bit - TODO configurable?

        if (priv->clock_output)
            LL_USART_EnableSCLKOutput(priv->periph);
        else
            LL_USART_DisableSCLKOutput(priv->periph);

        LL_USART_SetTransferBitOrder(priv->periph,
                                     priv->lsb_first ? LL_USART_BITORDER_LSBFIRST
                                                     : LL_USART_BITORDER_MSBFIRST);

        LL_USART_SetDataWidth(priv->periph,
                              priv->width == 7 ? LL_USART_DATAWIDTH_7B :
                              priv->width == 8 ? LL_USART_DATAWIDTH_8B
                                               : LL_USART_DATAWIDTH_9B);

        LL_USART_SetBinaryDataLogic(priv->periph,
                                    priv->data_inv ? LL_USART_BINARY_LOGIC_NEGATIVE
                                                   : LL_USART_BINARY_LOGIC_POSITIVE);

        LL_USART_SetRXPinLevel(priv->periph, priv->rx_inv ? LL_USART_RXPIN_LEVEL_INVERTED
                                                          : LL_USART_RXPIN_LEVEL_STANDARD);

        LL_USART_SetTXPinLevel(priv->periph, priv->tx_inv ? LL_USART_TXPIN_LEVEL_INVERTED
                                                          : LL_USART_TXPIN_LEVEL_STANDARD);

        if (priv->de_output)
            LL_USART_EnableDEMode(priv->periph);
        else
            LL_USART_DisableDEMode(priv->periph);

        LL_USART_SetDESignalPolarity(priv->periph,
                                     priv->de_polarity ? LL_USART_DE_POLARITY_HIGH
                                                       : LL_USART_DE_POLARITY_LOW);

        LL_USART_SetDEAssertionTime(priv->periph, priv->de_assert_time);
        LL_USART_SetDEDeassertionTime(priv->periph, priv->de_clear_time);
    }
    LL_USART_Enable(priv->periph);

    return E_SUCCESS;
}

/** Tear down the unit */
static void UUSART_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init the pins & peripheral only if inited correctly
    if (unit->status == E_SUCCESS) {
        assert_param(priv->periph);
        LL_USART_DeInit(priv->periph);
    }

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free(unit->data);
    unit->data = NULL;
}

// ------------------------------------------------------------------------

static error_t usart_wait_until_flag(struct priv *priv, uint32_t flag, bool stop_state)
{
    uint32_t t_start = HAL_GetTick();
    while (((priv->periph->ISR & flag) != 0) != stop_state) {
        if (HAL_GetTick() - t_start > 10) {
            dbg("ERR waiting for ISR flag 0x%p = %d", (void*)flag, (int)stop_state);
            return E_HW_TIMEOUT;
        }
    }
    return E_SUCCESS;
}

static error_t sync_send(struct priv *priv, const uint8_t *buf, uint32_t len)
{
    while (len > 0) {
        TRY(usart_wait_until_flag(priv, USART_ISR_TXE, 1));
        priv->periph->TDR = *buf++;
        len--;
    }
    TRY(usart_wait_until_flag(priv, USART_ISR_TC, 1));
    return E_SUCCESS;
}


enum PinCmd_ {
    CMD_WRITE = 0,
};

/** Handle a request message */
static error_t UUSART_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    switch (command) {
        case CMD_WRITE:
            dbg("Tx req. CR1 0x%p, CR2 0x%p, CR3 0x%p, BRR 0x%p",
                (void*)priv->periph->CR1,
                (void*)priv->periph->CR2,
                (void*)priv->periph->CR3,
                (void*)priv->periph->BRR);

            uint32_t len;
            const uint8_t *pld = pp_tail(pp, &len);

            TRY(sync_send(priv, pld, len));
            return E_SUCCESS;
            //return E_NOT_IMPLEMENTED;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_USART = {
    .name = "USART",
    .description = "Serial port",
    // Settings
    .preInit = UUSART_preInit,
    .cfgLoadBinary = UUSART_loadBinary,
    .cfgWriteBinary = UUSART_writeBinary,
    .cfgLoadIni = UUSART_loadIni,
    .cfgWriteIni = UUSART_writeIni,
    // Init
    .init = UUSART_init,
    .deInit = UUSART_deInit,
    // Function
    .handleRequest = UUSART_handleRequest,
};
