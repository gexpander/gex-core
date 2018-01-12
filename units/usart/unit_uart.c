//
// Created by MightyPork on 2018/01/02.
//

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

    uint32_t baudrate;  //!< UART baud rate
    uint8_t parity;     //!< 0-none, 1-odd, 2-even
    uint8_t stopbits;   //!< 1-one, 2-2, 3-1.5
    bool enable_rx;     //!< Configure and enable the Rx line
    bool enable_tx;     //!< Configure and enable the Tx line

    uint8_t hw_flow_control; //!< HW flow control 0-none, 1-RTX, 2-CTS, 3-both
    bool clock_output;       //!< Output serial clock
    bool cpol;          //!< clock CPOL setting
    bool cpha;          //!< clock CPHA setting

    uint32_t rx_buf_size; //!< Receive buffer size
    uint32_t rx_full_thr;  //!< Receive buffer full threshold (will be sent to PC)
    uint16_t rx_timeout;  //!< Receive timeout (time of inactivity before flushing to PC)

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
    priv->stopbits = 1;       //!< 1-one, 2-2, 3-1.5
    priv->enable_rx = true;
    priv->enable_tx = true;

    priv->hw_flow_control = false;
    priv->clock_output = false;
    priv->cpol = 0;
    priv->cpha = 0;

    priv->rx_buf_size = 128;
    priv->rx_full_thr = 90;
    priv->rx_timeout = 3; // ms

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
    priv->remap = pp_u8(pp);

    priv->baudrate = pp_u32(pp);
    priv->parity = pp_u8(pp);
    priv->stopbits = pp_u8(pp);
    priv->enable_rx = pp_bool(pp);
    priv->enable_tx = pp_bool(pp);

    priv->hw_flow_control = pp_u8(pp);
    priv->clock_output = pp_bool(pp);
    priv->cpol = pp_bool(pp);
    priv->cpha = pp_bool(pp);

    priv->rx_buf_size = pp_u32(pp);
    priv->rx_full_thr = pp_u32(pp);
    priv->rx_timeout = pp_u16(pp);
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
    pb_bool(pb, priv->enable_rx);
    pb_bool(pb, priv->enable_tx);

    pb_u8(pb, priv->hw_flow_control);
    pb_bool(pb, priv->clock_output);
    pb_bool(pb, priv->cpol);
    pb_bool(pb, priv->cpha);

    pb_u32(pb, priv->rx_buf_size);
    pb_u32(pb, priv->rx_full_thr);
    pb_u16(pb, priv->rx_timeout);
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
        priv->parity = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "stop-bits")) {
        priv->stopbits = (uint8_t) (1 + str_parse_012(value, "1", "2", "1.5", &suc));
    }
    else if (streq(key, "enable-rx")) {
        priv->enable_rx = str_parse_yn(value, &suc);
    }
    else if (streq(key, "enable-tx")) {
        priv->enable_tx = str_parse_yn(value, &suc);
    }
    else if (streq(key, "hw-flow-control")) {
        priv->hw_flow_control = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "synchronous")) {
        priv->clock_output = (bool) avr_atoi(value);
    }
    else if (streq(key, "cpol")) {
        priv->cpol = (bool) avr_atoi(value);
    }
    else if (streq(key, "cpha")) {
        priv->cpha = (bool) avr_atoi(value);
    }
    else if (streq(key, "rx-buffer")) {
        priv->rx_buf_size = (uint32_t ) avr_atoi(value);
    }
    else if (streq(key, "rx-full-theshold")) {
        priv->rx_full_thr = (uint32_t ) avr_atoi(value);
    }
    else if (streq(key, "rx-timeout")) {
        priv->rx_timeout = (uint16_t ) avr_atoi(value);
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

    iw_comment(iw, "Peripheral number (UARTx)");
    iw_entry(iw, "device", "%d", (int)priv->periph_num);

    iw_comment(iw, "Pin mappings (SCK,MISO,MOSI)");
#if GEX_PLAT_F072_DISCOVERY
    // TODO
//    iw_comment(iw, " SPI1: (0) A5,A6,A7     (1) B3,B4,B5  (2) E13,E14,E15");
//    iw_comment(iw, " SPI2: (0) B13,B14,B15  (1) D1,D3,D4");
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
    iw_comment(iw, "Baud rate"); // TODO examples/range
    iw_entry(iw, "baud-rate", "%d", (int)priv->baudrate);

    iw_comment(iw, "Parity (0-none, 1-odd, 2-even)");
    iw_entry(iw, "parity", "%d", (int)priv->parity);

    iw_comment(iw, "Stop-bits (1, 1.5, 2)");
    iw_entry(iw, "stop-bits", "%s", (priv->stopbits==3)?"1.5":(priv->stopbits==2?"2":"1"));

    iw_comment(iw, "Enable the RX / TX lines");
    iw_entry(iw, "exable-rx", str_yn(priv->enable_rx));
    iw_entry(iw, "exable-tx", str_yn(priv->enable_tx));

    iw_comment(iw, "Hardware flow control (NONE, RTS, CTS, ALL)");
    iw_entry(iw, "hw-flow-control", "%s", str_4(priv->hw_flow_control, 0, "NONE", 1, "RTS", 2, "CTS", 3, "ALL"));

    iw_comment(iw, "Clock output (Y,N) - N for async");
    iw_entry(iw, "clock-output", str_yn(priv->clock_output));
    iw_comment(iw, "Sync: Clock polarity: 0,1 (clock idle level)");
    iw_entry(iw, "cpol", "%d", (int)priv->cpol);
    iw_comment(iw, "Sync: Clock phase: 0,1 (active edge, 0-first, 1-second)");
    iw_entry(iw, "cpha", "%d", (int)priv->cpha);

    iw_comment(iw, "Receive buffer capacity, full threshold and flush timeout");
    iw_entry(iw, "rx-buffer", "%d", (int)priv->rx_buf_size);
    iw_entry(iw, "rx-full-theshold", "%d", (int)priv->rx_full_thr);
    iw_entry(iw, "rx-timeout", "%d", (int)priv->rx_timeout);
}

// ------------------------------------------------------------------------


/** Finalize unit set-up */
static error_t UUSART_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (!(priv->periph_num >= 1 && priv->periph_num <= 4)) {
        dbg("!! Bad UART periph");
        // TODO count based on platform chip
        return E_BAD_CONFIG;
    }

    // assign and claim the peripheral
    if (priv->periph_num == 1) {
        TRY(rsc_claim(unit, R_USART1));
        priv->periph = USART1;
    }
    else if (priv->periph_num == 2) {
        TRY(rsc_claim(unit, R_USART2));
        priv->periph = USART2;
    }
    else if (priv->periph_num == 3) {
        TRY(rsc_claim(unit, R_USART3));
        priv->periph = USART3;
    }
    else if (priv->periph_num == 4) {
        TRY(rsc_claim(unit, R_USART4));
        priv->periph = USART4;
    }

    // This is written for F072, other platforms will need adjustments

    // Configure UART pins (AF)

    char uart_portname;
    uint8_t pin_rx;
    uint8_t pin_tx;
    uint8_t pin_clk;
    uint8_t pin_rts;
    uint8_t pin_cts;
    uint32_t af_uart;

    // TODO
#if GEX_PLAT_F072_DISCOVERY
    // SPI1 - many options
    // sck, miso, mosi, af

    if (priv->periph_num == 1) {
        // SPI1
        if (priv->remap == 0) {
            uart_portname = 'A';
//            af_spi = LL_GPIO_AF_0;
//            pin_sck = 5;
//            pin_miso = 6;
//            pin_mosi = 7;
        }
        else {
            return E_BAD_CONFIG;
        }
    }
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
//
//    // first, we have to claim the pins
//    TRY(rsc_claim_pin(unit, uart_portname, pin_mosi));
//    TRY(rsc_claim_pin(unit, uart_portname, pin_miso));
//    TRY(rsc_claim_pin(unit, uart_portname, pin_sck));
//
//    configure_gpio_alternate(uart_portname, pin_mosi, af_spi);
//    configure_gpio_alternate(uart_portname, pin_miso, af_spi);
//    configure_gpio_alternate(uart_portname, pin_sck, af_spi);
//
//    if (priv->periph_num == 1) {
//        __HAL_RCC_SPI1_CLK_ENABLE();
//    } else {
//        __HAL_RCC_SPI2_CLK_ENABLE();
//    }
//
//    // configure SSN GPIOs
//    {
//        // Claim all needed pins
//        TRY(rsc_claim_gpios(unit, priv->ssn_port_name, priv->ssn_pins));
//        TRY(configure_sparse_pins(priv->ssn_port_name, priv->ssn_pins, &priv->ssn_port,
//                                  LL_GPIO_MODE_OUTPUT, LL_GPIO_OUTPUT_PUSHPULL));
//        // Set the initial state - all high
//        priv->ssn_port->BSRR = priv->ssn_pins;
//    }
//
//    // Configure SPI - must be configured under reset
//    LL_SPI_Disable(priv->periph);
//    {
//        uint32_t presc = priv->prescaller;
//        uint32_t lz = __CLZ(presc);
//        if (lz < 23) lz = 23;
//        if (lz > 30) lz = 30;
//        presc = (32 - lz - 2);
//        LL_SPI_SetBaudRatePrescaler(priv->periph, (presc<<SPI_CR1_BR_Pos)&SPI_CR1_BR_Msk);
//
//        LL_SPI_SetClockPolarity(priv->periph,     priv->cpol ? LL_SPI_POLARITY_HIGH : LL_SPI_POLARITY_LOW);
//        LL_SPI_SetClockPhase(priv->periph,        priv->cpha ? LL_SPI_PHASE_1EDGE : LL_SPI_PHASE_2EDGE);
//        LL_SPI_SetTransferDirection(priv->periph, priv->tx_only ? LL_SPI_HALF_DUPLEX_TX : LL_SPI_FULL_DUPLEX);
//        LL_SPI_SetTransferBitOrder(priv->periph,  priv->lsb_first ? LL_SPI_LSB_FIRST : LL_SPI_MSB_FIRST);
//
//        LL_SPI_SetNSSMode(priv->periph, LL_SPI_NSS_SOFT);
//        LL_SPI_SetDataWidth(priv->periph, LL_SPI_DATAWIDTH_8BIT);
//        LL_SPI_SetRxFIFOThreshold(priv->periph, LL_SPI_RX_FIFO_TH_QUARTER); // trigger RXNE on 1 byte
//
//        LL_SPI_SetMode(priv->periph, LL_SPI_MODE_MASTER);
//    }
//    LL_SPI_Enable(priv->periph);

    return E_SUCCESS;
}

/** Tear down the unit */
static void UUSART_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

//    // de-init the pins & peripheral only if inited correctly
//    if (unit->status == E_SUCCESS) {
//        assert_param(priv->periph);
//
//        LL_SPI_DeInit(priv->periph);
//
//        if (priv->periph_num == 1) {
//            __HAL_RCC_SPI1_CLK_DISABLE();
//        } else {
//            __HAL_RCC_SPI2_CLK_DISABLE();
//        }
//    }

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free(unit->data);
    unit->data = NULL;
}

// ------------------------------------------------------------------------

enum PinCmd_ {
    CMD_WRITE = 0,
};

/** Handle a request message */
static error_t UUSART_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    switch (command) {
        case CMD_WRITE:
            return E_NOT_IMPLEMENTED;

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
