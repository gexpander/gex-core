//
// Created by MightyPork on 2018/01/02.
//

#include "comm/messages.h"
#include "unit_base.h"
#include "utils/avrlibc.h"
#include "unit_spi.h"

// SPI master

/** Private data structure */
struct priv {
    uint8_t periph_num; //!< 1 or 2
    uint16_t prescaller; //!< Clock prescaller, stored as the dividing factor

    bool cpol;          //!< CPOL setting
    bool cpha;          //!< CPHA setting
    bool tx_only;       //!< If true, Enable only the MOSI line
    bool lsb_first;     //!< Option to send LSB first

    char ssn_port_name; //!< SSN port
    uint16_t ssn_pins;  //!< SSN pin mask

    SPI_TypeDef *periph;

    GPIO_TypeDef *ssn_port;
    GPIO_TypeDef *spi_port;
    uint32_t ll_pin_miso;
    uint32_t ll_pin_mosi;
    uint32_t ll_pin_sck;
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
    else if (streq(key, "lsb-first")) {
        priv->lsb_first = str_parse_yn(value, &suc);
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

    iw_comment(iw, "Prescaller: 2,4,8,...,256");
    iw_entry(iw, "prescaller", "%d", (int)priv->prescaller);

    iw_comment(iw, "Clock polarity: 0,1 (clock idle level)");
    iw_entry(iw, "cpol", "%d", (int)priv->cpol);

    iw_comment(iw, "Clock phase: 0,1 (active edge, 0-first, 1-second)");
    iw_entry(iw, "cpha", "%d", (int)priv->cpha);

    iw_comment(iw, "Transmit only, disable MISO");
    iw_entry(iw, "tx-only", str_yn(priv->tx_only));

    iw_comment(iw, "Use LSB-first bit order");
    iw_entry(iw, "lsb-first", str_yn(priv->lsb_first));

    iw_comment(iw, "Slave Select port name");
    iw_entry(iw, "port", "%c", priv->ssn_port_name);

    iw_comment(iw, "Slave select pins (comma separated, supports ranges)");
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
    #error "NO IMPL"
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
    Resource r_mosi = pin2resource(spi_portname, pin_mosi, &suc);
    Resource r_miso = pin2resource(spi_portname, pin_miso, &suc);
    Resource r_sck = pin2resource(spi_portname, pin_sck, &suc);
    if (!suc) return E_BAD_CONFIG;

    TRY(rsc_claim(unit, r_mosi));
    TRY(rsc_claim(unit, r_miso));
    TRY(rsc_claim(unit, r_sck));

    priv->spi_port = port2periph(spi_portname, &suc);
    priv->ll_pin_mosi = pin2ll(pin_mosi, &suc);
    priv->ll_pin_miso = pin2ll(pin_miso, &suc);
    priv->ll_pin_sck = pin2ll(pin_sck, &suc);
    if (!suc) return E_BAD_CONFIG;

    // configure AF
    if (pin_mosi < 8) LL_GPIO_SetAFPin_0_7(priv->spi_port, priv->ll_pin_mosi, af_spi);
    else LL_GPIO_SetAFPin_8_15(priv->spi_port, priv->ll_pin_mosi, af_spi);

    if (pin_miso < 8) LL_GPIO_SetAFPin_0_7(priv->spi_port, priv->ll_pin_miso, af_spi);
    else LL_GPIO_SetAFPin_8_15(priv->spi_port, priv->ll_pin_miso, af_spi);

    if (pin_sck < 8) LL_GPIO_SetAFPin_0_7(priv->spi_port, priv->ll_pin_sck, af_spi);
    else LL_GPIO_SetAFPin_8_15(priv->spi_port, priv->ll_pin_sck, af_spi);

    LL_GPIO_SetPinMode(priv->spi_port, priv->ll_pin_mosi, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(priv->spi_port, priv->ll_pin_miso, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(priv->spi_port, priv->ll_pin_sck, LL_GPIO_MODE_ALTERNATE);

    if (priv->periph_num == 1) {
        __HAL_RCC_SPI1_CLK_ENABLE();
    } else {
        __HAL_RCC_SPI2_CLK_ENABLE();
    }

    // configure SSN GPIOs
    {
        priv->ssn_port = port2periph(priv->ssn_port_name, &suc);
        if (!suc) return E_BAD_CONFIG;

        // Claim all needed pins
        TRY(rsc_claim_gpios(unit, priv->ssn_port_name, priv->ssn_pins));

        uint16_t mask = 1;
        for (int i = 0; i < 16; i++, mask <<= 1) {
            if (priv->ssn_pins & mask) {
                uint32_t ll_pin = pin2ll((uint8_t) i, &suc);
                LL_GPIO_SetPinMode(priv->ssn_port, ll_pin, LL_GPIO_MODE_OUTPUT);
                LL_GPIO_SetPinOutputType(priv->ssn_port, ll_pin, LL_GPIO_OUTPUT_PUSHPULL);
                LL_GPIO_SetPinSpeed(priv->ssn_port, ll_pin, LL_GPIO_SPEED_FREQ_HIGH);
            }
        }

        // Set the initial state - all high
        priv->ssn_port->ODR &= priv->ssn_pins;
    }

    // Configure SPI - must be configured under reset
    LL_SPI_Disable(priv->periph);
    {
        uint32_t presc = priv->prescaller;
        uint32_t lz = __CLZ(presc);
        if (lz < 23) lz = 23;
        if (lz > 30) lz = 30;
        presc = (32 - lz - 2);
        dbg("Presc is %d", (int) presc);
        LL_SPI_SetBaudRatePrescaler(priv->periph, presc);

        LL_SPI_SetClockPolarity(priv->periph, priv->cpol ?
                                              LL_SPI_POLARITY_HIGH :
                                              LL_SPI_POLARITY_LOW);

        LL_SPI_SetClockPhase(priv->periph, priv->cpha ?
                                           LL_SPI_PHASE_1EDGE :
                                           LL_SPI_PHASE_2EDGE);

        LL_SPI_SetTransferDirection(priv->periph, priv->tx_only ?
                                                  LL_SPI_HALF_DUPLEX_TX :
                                                  LL_SPI_FULL_DUPLEX);

        LL_SPI_SetTransferBitOrder(priv->periph, priv->lsb_first ?
                                                 LL_SPI_LSB_FIRST :
                                                 LL_SPI_MSB_FIRST);

        // TODO data size?

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
        assert_param(priv->spi_port);
        assert_param(priv->ssn_port);

        LL_SPI_DeInit(priv->periph);

        if (priv->periph_num == 1) {
            __HAL_RCC_SPI1_CLK_DISABLE();
        } else {
            __HAL_RCC_SPI2_CLK_DISABLE();
        }

        LL_GPIO_SetPinMode(priv->spi_port, priv->ll_pin_mosi, LL_GPIO_MODE_ANALOG);
        LL_GPIO_SetPinMode(priv->spi_port, priv->ll_pin_mosi, LL_GPIO_MODE_ANALOG);
        LL_GPIO_SetPinMode(priv->spi_port, priv->ll_pin_sck, LL_GPIO_MODE_ANALOG);

        // de-init all SSN pins
        bool suc = true;
        uint16_t mask = 1;
        for (int i = 0; i < 16; i++, mask <<= 1) {
            if (priv->ssn_pins & mask) {
                uint32_t ll_pin = pin2ll((uint8_t) i, &suc);
                assert_param(suc);
                // configure the pin as analog
                LL_GPIO_SetPinMode(priv->ssn_port, ll_pin, LL_GPIO_MODE_ANALOG);
            }
        }
    }

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
static error_t USPI_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    uint16_t addr;
    uint32_t len;
    uint8_t regnum;
    uint32_t size;

    // NOTE: 10-bit addresses must have the highest bit set to 1 for indication (0x8000 | addr)

    switch (command) {
        /** Write byte(s) - addr:u16, byte(s)  */
        case CMD_WRITE:
            addr = pp_u16(pp);
            const uint8_t *bb = pp_tail(pp, &len);

            return UU_SPI_Write(unit, addr, bb, len); // TODo implement function + SPI pins mapping

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
