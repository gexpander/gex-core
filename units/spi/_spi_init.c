//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define SPI_INTERNAL
#include "_spi_internal.h"

/** Allocate data structure and set defaults */
error_t USPI_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

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
error_t USPI_init(Unit *unit)
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
#if STM32F072xB
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
//        else if (priv->remap == 2) {
//            // large packages only
//            spi_portname = 'E';
//            af_spi = LL_GPIO_AF_1;
//            pin_sck = 13;
//            pin_miso = 14;
//            pin_mosi = 15;
//        }
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
//        else if (priv->remap == 1) {
//            // NOTE: there's also an incomplete remap in PB and PC
//            spi_portname = 'D';
//            af_spi = LL_GPIO_AF_0;
//            pin_sck = 1;
//            pin_miso = 3;
//            pin_mosi = 4;
//        }
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

    TRY(hw_configure_gpio_af(spi_portname, pin_mosi, af_spi));
    TRY(hw_configure_gpio_af(spi_portname, pin_miso, af_spi));
    TRY(hw_configure_gpio_af(spi_portname, pin_sck, af_spi));

    // configure SSN GPIOs
    {
        // Claim all needed pins
        TRY(rsc_claim_gpios(unit, priv->ssn_port_name, priv->ssn_pins));
        TRY(hw_configure_sparse_pins(priv->ssn_port_name, priv->ssn_pins, &priv->ssn_port,
                                     LL_GPIO_MODE_OUTPUT, LL_GPIO_OUTPUT_PUSHPULL));
        // Set the initial state - all high
        priv->ssn_port->BSRR = priv->ssn_pins;
    }

    hw_periph_clock_enable(priv->periph);

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
void USPI_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init the pins & peripheral only if inited correctly
    if (unit->status == E_SUCCESS) {
        assert_param(priv->periph);
        LL_SPI_DeInit(priv->periph);

        hw_periph_clock_disable(priv->periph);
    }

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
