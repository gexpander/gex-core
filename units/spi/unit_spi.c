//
// Created by MightyPork on 2018/01/02.
//

#include <stm32f072xb.h>
#include "comm/messages.h"
#include "unit_base.h"
#include "utils/avrlibc.h"
#include "unit_spi.h"

// SPI master

/** Private data structure */
struct priv {
    uint8_t periph_num; //!< 1 or 2
    uint8_t remap;      //!< SPI remap option

    uint16_t prescaller; //!< Clock prescaller, stored as the dividing factor
    bool cpol;          //!< CPOL setting
    bool cpha;          //!< CPHA setting
    bool tx_only;       //!< If true, Enable only the MOSI line

    bool lsb_first;     //!< Option to send LSB first
    char ssn_port_name; //!< SSN port
    uint16_t ssn_pins;  //!< SSN pin mask

    SPI_TypeDef *periph;
    GPIO_TypeDef *ssn_port;
};

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
static void USPI_loadBinary(Unit *unit, PayloadParser *pp)
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
static void USPI_writeBinary(Unit *unit, PayloadBuilder *pb)
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
static error_t USPI_loadIni(Unit *unit, const char *key, const char *value)
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
        suc = parse_port(value, &priv->ssn_port_name);
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
static void USPI_writeIni(Unit *unit, IniWriter *iw)
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
    iw_entry(iw, "pins", "%s", str_pinmask(priv->ssn_pins, unit_tmp512));
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static error_t USPI_preInit(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv), &suc);
    if (!suc) return E_OUT_OF_MEM;

    // some defaults
    priv->periph_num = 1;
    priv->prescaller = 64;
    priv->remap = 0;

    priv->cpol = 0;
    priv->cpha = 0;
    priv->tx_only = false;
    priv->lsb_first = false;

    priv->ssn_port_name = 'A';
    priv->ssn_pins = 0x0001;

    return E_SUCCESS;
}

/** Finalize unit set-up */
static error_t USPI_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (!(priv->periph_num >= 1 && priv->periph_num <= 2)) {
        dbg("!! Bad SPI periph");
        // XXX some chips have also SPI3
        return E_BAD_CONFIG;
    }


    // assign and claim the peripheral
    if (priv->periph_num == 1) {
        TRY(rsc_claim(unit, R_SPI1));
        priv->periph = SPI1;
    }
    else if (priv->periph_num == 2) {
        TRY(rsc_claim(unit, R_SPI2));
        priv->periph = SPI2;
    }

    // This is written for F072, other platforms will need adjustments

    // Configure SPI own pins (AF)

    char spi_portname;
    uint8_t pin_miso;
    uint8_t pin_mosi;
    uint8_t pin_sck;
    uint32_t af_spi;

    // TODO
#if GEX_PLAT_F072_DISCOVERY
    // SPI1 - many options
    // sck, miso, mosi, af

    if (priv->periph_num == 1) {
        // SPI1
        if (priv->remap == 0) {
            spi_portname = 'A';
            af_spi = LL_GPIO_AF_0;
            pin_sck = 5;
            pin_miso = 6;
            pin_mosi = 7;
        }
        else if (priv->remap == 1) {
            spi_portname = 'B';
            af_spi = LL_GPIO_AF_0;
            pin_sck = 3;
            pin_miso = 4;
            pin_mosi = 5;
        }
        else if (priv->remap == 2) {
            // large packages only
            spi_portname = 'E';
            af_spi = LL_GPIO_AF_1;
            pin_sck = 13;
            pin_miso = 14;
            pin_mosi = 15;
        }
        else {
            return E_BAD_CONFIG;
        }
    }
    else {
        // SPI2
        if (priv->remap == 0) {
            spi_portname = 'B';
            af_spi = LL_GPIO_AF_0;
            pin_sck = 13;
            pin_miso = 14;
            pin_mosi = 15;
        }
        else if (priv->remap == 1) {
            // NOTE: the's also a incomplete remap in PB and PC
            spi_portname = 'D';
            af_spi = LL_GPIO_AF_0;
            pin_sck = 1;
            pin_miso = 3;
            pin_mosi = 4;
        }
        else {
            return E_BAD_CONFIG;
        }
    }

#elif GEX_PLAT_F103_BLUEPILL
    #error "NO IMPL"
#elif GEX_PLAT_F303_DISCOVERY
    #error "NO IMPL"
#elif GEX_PLAT_F407_DISCOVERY
    #error "NO IMPL"
#else
    #error "BAD PLATFORM!"
#endif

    // first, we have to claim the pins
    TRY(rsc_claim_pin(unit, spi_portname, pin_mosi));
    TRY(rsc_claim_pin(unit, spi_portname, pin_miso));
    TRY(rsc_claim_pin(unit, spi_portname, pin_sck));

    configure_gpio_alternate(spi_portname, pin_mosi, af_spi);
    configure_gpio_alternate(spi_portname, pin_miso, af_spi);
    configure_gpio_alternate(spi_portname, pin_sck, af_spi);

    // configure SSN GPIOs
    {
        // Claim all needed pins
        TRY(rsc_claim_gpios(unit, priv->ssn_port_name, priv->ssn_pins));
        TRY(configure_sparse_pins(priv->ssn_port_name, priv->ssn_pins, &priv->ssn_port,
                                  LL_GPIO_MODE_OUTPUT, LL_GPIO_OUTPUT_PUSHPULL));
        // Set the initial state - all high
        priv->ssn_port->BSRR = priv->ssn_pins;
    }

    if (priv->periph_num == 1) {
        __HAL_RCC_SPI1_CLK_ENABLE();
    } else {
        __HAL_RCC_SPI2_CLK_ENABLE();
    }

    // Configure SPI - must be configured under reset
    LL_SPI_Disable(priv->periph);
    {
        uint32_t presc = priv->prescaller;
        uint32_t lz = __CLZ(presc);
        if (lz < 23) lz = 23;
        if (lz > 30) lz = 30;
        presc = (32 - lz - 2);
        LL_SPI_SetBaudRatePrescaler(priv->periph, (presc<<SPI_CR1_BR_Pos)&SPI_CR1_BR_Msk);

        LL_SPI_SetClockPolarity(priv->periph,     priv->cpol ? LL_SPI_POLARITY_HIGH : LL_SPI_POLARITY_LOW);
        LL_SPI_SetClockPhase(priv->periph,        priv->cpha ? LL_SPI_PHASE_1EDGE : LL_SPI_PHASE_2EDGE);
        LL_SPI_SetTransferDirection(priv->periph, priv->tx_only ? LL_SPI_HALF_DUPLEX_TX : LL_SPI_FULL_DUPLEX);
        LL_SPI_SetTransferBitOrder(priv->periph,  priv->lsb_first ? LL_SPI_LSB_FIRST : LL_SPI_MSB_FIRST);

        LL_SPI_SetNSSMode(priv->periph, LL_SPI_NSS_SOFT);
        LL_SPI_SetDataWidth(priv->periph, LL_SPI_DATAWIDTH_8BIT);
        LL_SPI_SetRxFIFOThreshold(priv->periph, LL_SPI_RX_FIFO_TH_QUARTER); // trigger RXNE on 1 byte

        LL_SPI_SetMode(priv->periph, LL_SPI_MODE_MASTER);
    }
    LL_SPI_Enable(priv->periph);

    return E_SUCCESS;
}

/** Tear down the unit */
static void USPI_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init the pins & peripheral only if inited correctly
    if (unit->status == E_SUCCESS) {
        assert_param(priv->periph);
        LL_SPI_DeInit(priv->periph);

        if (priv->periph_num == 1) {
            __HAL_RCC_SPI1_CLK_DISABLE();
        } else {
            __HAL_RCC_SPI2_CLK_DISABLE();
        }
    }

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free(unit->data);
    unit->data = NULL;
}

// ------------------------------------------------------------------------

static error_t spi_wait_until_flag(struct priv *priv, uint32_t flag, bool stop_state)
{
    uint32_t t_start = HAL_GetTick();
    while (((priv->periph->SR & flag) != 0) != stop_state) {
        if (HAL_GetTick() - t_start > 10) {
            return E_HW_TIMEOUT;
        }
    }
    return E_SUCCESS;
}

/**
 * Perform a low level SPI transfer
 *
 * @param priv - private object of the SPI unit
 * @param request - request buffer
 * @param response - response buffer
 * @param req_len - request len
 * @param resp_skip - response skip bytes
 * @param resp_len - response len
 * @return success
 */
static error_t xfer_do(struct priv *priv, const uint8_t *request,
                       uint8_t *response,
                       uint32_t req_len,
                       uint32_t resp_skip,
                       uint32_t resp_len)
{
    // TODO this is slow, use DMA

    if (response == NULL) resp_len = 0;

    // avoid skip causing stretch beyond tx window if nothing is to be read back
    if (resp_len == 0) resp_skip = 0;

    // in tx only mode, return zeros
    if (priv->tx_only && resp_len>0) {
        memset(response, 0, resp_len);
    }

    uint8_t tb;
    uint32_t end = MAX(req_len, resp_len + resp_skip);
    for (uint32_t i = 0; i < end; i++) {
        if (i < req_len) tb = *request++;
        else tb = 0;

        TRY(spi_wait_until_flag(priv, SPI_SR_TXE, true));
        LL_SPI_TransmitData8(priv->periph, tb);

        if (!priv->tx_only) {
            TRY(spi_wait_until_flag(priv, SPI_SR_RXNE, true));
            uint8_t rb = LL_SPI_ReceiveData8(priv->periph);

            if (resp_skip > 0) resp_skip--;
            else if (resp_len > 0) {
                resp_len--;
                *response++ = rb;
            }
        }
    }
    return E_SUCCESS;
}

error_t UU_SPI_Multicast(Unit *unit, uint16_t slaves,
                     const uint8_t *request, uint32_t req_len)
{
    struct priv *priv= unit->data;
    uint16_t mask = port_spread(slaves, priv->ssn_pins);
    priv->ssn_port->BRR = mask;
    {
        TRY(xfer_do(priv, request, NULL, req_len, 0, 0));
    }
    priv->ssn_port->BSRR = mask;

    return E_SUCCESS;
}

error_t UU_SPI_Write(Unit *unit, uint8_t slave_num,
                     const uint8_t *request, uint8_t *response,
                     uint32_t req_len, uint32_t resp_skip, uint32_t resp_len)
{
    struct priv *priv= unit->data;

    uint16_t mask = port_spread((uint16_t) (1 << slave_num), priv->ssn_pins);
    priv->ssn_port->BRR = mask;
    {
        TRY(xfer_do(priv, request, response, req_len, resp_len, resp_skip));
    }
    priv->ssn_port->BSRR = mask;

    return E_SUCCESS;
}


enum PinCmd_ {
    CMD_QUERY = 0,
    CMD_MULTICAST = 1,
};

/** Handle a request message */
static error_t USPI_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    uint8_t slave;
    uint16_t slaves;
    uint16_t req_len;
    uint16_t resp_skip;
    uint16_t resp_len;
    const uint8_t *bb;

    uint32_t len;

    switch (command) {
        /** Write and read byte(s) - slave_num:u8, req_len:u16, resp_skip:u16, resp_len:u16, byte(s)  */
        case CMD_QUERY:
            slave = pp_u8(pp);
            resp_skip = pp_u16(pp);
            resp_len = pp_u16(pp);

            bb = pp_tail(pp, &len);

            TRY(UU_SPI_Write(unit, slave,
                             bb, (uint8_t *) unit_tmp512,
                             len, resp_skip, resp_len));

            // no response if we aren't reading
            if (resp_len > 0) {
                com_respond_buf(frame_id, MSG_SUCCESS, (uint8_t *) unit_tmp512, resp_len);
            }
            return E_SUCCESS;

        /** Write byte(s) to multiple slaves - slaves:u16, req_len:u16, byte(s)  */
        case CMD_MULTICAST:
            slaves = pp_u16(pp);

            bb = pp_tail(pp, &len);

            TRY(UU_SPI_Multicast(unit, slaves, bb, len));
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_SPI = {
    .name = "SPI",
    .description = "SPI master",
    // Settings
    .preInit = USPI_preInit,
    .cfgLoadBinary = USPI_loadBinary,
    .cfgWriteBinary = USPI_writeBinary,
    .cfgLoadIni = USPI_loadIni,
    .cfgWriteIni = USPI_writeIni,
    // Init
    .init = USPI_init,
    .deInit = USPI_deInit,
    // Function
    .handleRequest = USPI_handleRequest,
};
