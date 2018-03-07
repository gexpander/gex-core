//
// Created by MightyPork on 2018/01/14.
//
#include "platform.h"
#include "unit_base.h"

#define UUSART_INTERNAL
#include "_usart_internal.h"

extern error_t UUSART_ClaimDMAs(Unit *unit);
extern error_t UUSART_SetupDMAs(Unit *unit);
extern void UUSART_DeInitDMAs(Unit *unit);

/** Allocate data structure and set defaults */
error_t UUSART_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    // some defaults
    priv->periph_num = 1;
    priv->remap = 0;

    priv->baudrate = 115200;
    priv->parity = 0;         //!< 0-none, 1-odd, 2-even
    priv->stopbits = 1;       //!< 0-half, 1-one, 2-1.5, 3-two
    priv->direction = UUSART_DIRECTION_RXTX; // RXTX

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

/** Claim the peripheral and assign priv->periph */
static inline error_t UUSART_claimPeriph(Unit *unit)
{
    struct priv *priv = unit->data;

    if (!(priv->periph_num >= 1 && priv->periph_num <= 5)) {
        dbg("!! Bad USART periph");
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
#if defined(USART4)
    else if (priv->periph_num == 4) {
        TRY(rsc_claim(unit, R_USART4));
        priv->periph = USART4;
    }
#endif
#if defined(USART5)
        else if (priv->periph_num == 5) {
        TRY(rsc_claim(unit, R_USART5));
        priv->periph = USART5;
    }
#endif
    else return E_BAD_CONFIG;

    TRY(UUSART_ClaimDMAs(unit));

    return E_SUCCESS;
}

/** Claim and configure GPIOs used */
static inline error_t UUSART_configPins(Unit *unit)
{
    struct priv *priv = unit->data;
    // This is written for F072, other platforms will need adjustments

    // Configure UART pins (AF)

#define want_ck_pin(priv) ((priv)->clock_output)
#define want_tx_pin(priv) (bool)((priv)->direction & 2)
#define want_rx_pin(priv) (bool)((priv)->direction & 1)
#define want_cts_pin(priv) ((priv)->hw_flow_control==2 || (priv)->hw_flow_control==3)
#define want_rts_pin(priv) ((priv)->de_output || (priv)->hw_flow_control==1 || (priv)->hw_flow_control==3)

    /* List of required pins based on the user config */
    bool pins_wanted[5] = {
        want_ck_pin(priv),
        want_tx_pin(priv),
        want_rx_pin(priv),
        want_cts_pin(priv),
        want_rts_pin(priv)
    };

#if STM32F072xB

    const struct PinAF *mappings = NULL;

    // TODO adjust this, possibly remove / split to individual pin config for ..
    // the final board

    const struct PinAF mapping_1_0[5] = {
        {'A', 8, LL_GPIO_AF_1}, // CK
        {'A', 9, LL_GPIO_AF_1}, // TX
        {'A', 10, LL_GPIO_AF_1}, // RX
        {'A', 11, LL_GPIO_AF_1}, // CTS - collides with USB
        {'A', 12, LL_GPIO_AF_1}, // RTS - collides with USB
    };

    const struct PinAF mapping_1_1[5] = {
        {'A', 8, LL_GPIO_AF_1}, // CK*
        {'B', 6, LL_GPIO_AF_1}, // TX
        {'B', 7, LL_GPIO_AF_1}, // RX
        {'A', 11, LL_GPIO_AF_1}, // CTS* - collides with USB
        {'A', 12, LL_GPIO_AF_1}, // RTS* - collides with USB
    };

    const struct PinAF mapping_2_0[5] = {
        {'A', 4, LL_GPIO_AF_1}, // CK
        {'A', 2, LL_GPIO_AF_1}, // TX
        {'A', 3, LL_GPIO_AF_1}, // RX
        {'A', 0, LL_GPIO_AF_1}, // CTS
        {'A', 1, LL_GPIO_AF_1}, // RTS
    };

    const struct PinAF mapping_2_1[5] = {
        {'A', 4, LL_GPIO_AF_1}, // CK*
        {'A', 14, LL_GPIO_AF_1}, // TX
        {'A', 15, LL_GPIO_AF_1}, // RX
        {'A', 0, LL_GPIO_AF_1}, // CTS*
        {'A', 1, LL_GPIO_AF_1}, // RTS*
    };

    const struct PinAF mapping_3_0[5] = {
        {'B', 12, LL_GPIO_AF_4}, // CK
        {'B', 10, LL_GPIO_AF_4}, // TX
        {'B', 11, LL_GPIO_AF_4}, // RX
        {'B', 13, LL_GPIO_AF_4}, // CTS
        {'B', 14, LL_GPIO_AF_4}, // RTS
    };

    const struct PinAF mapping_4_0[5] = {
        {'C', 12, LL_GPIO_AF_0}, // CK
        {'A', 0, LL_GPIO_AF_4}, // TX
        {'A', 1, LL_GPIO_AF_4}, // RX
        {'B', 7, LL_GPIO_AF_4}, // CTS
        {'A', 15, LL_GPIO_AF_4}, // RTS
    };

    const struct PinAF mapping_4_1[5] = {
        {'C', 12, LL_GPIO_AF_0}, // CK*
        {'C', 10, LL_GPIO_AF_0}, // TX
        {'C', 11, LL_GPIO_AF_0}, // RX
        {'B', 7,  LL_GPIO_AF_4}, // CTS*
        {'A', 15, LL_GPIO_AF_4}, // RTS*
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
        else return E_BAD_CONFIG;
    }
    else if (priv->periph_num == 4) {
        // USART3
        if (priv->remap == 0) mappings = &mapping_4_0[0];
        else if (priv->remap == 1) mappings = &mapping_4_1[0];
        else return E_BAD_CONFIG;
    }
    else return E_BAD_CONFIG;

    // Apply mappings based on the 'wanted' table
    for (int i = 0; i < 5; i++) {
        if (pins_wanted[i]) {
            if (mappings[i].port == 0) return E_BAD_CONFIG;
            TRY(rsc_claim_pin(unit, mappings[i].port, mappings[i].pin));
            TRY(hw_configure_gpio_af(mappings[i].port, mappings[i].pin, mappings[i].af));
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

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t UUSART_init(Unit *unit)
{
    struct priv *priv = unit->data;

    TRY(UUSART_claimPeriph(unit));
    TRY(UUSART_configPins(unit));

    // --- Configure the peripheral ---

    // Enable clock for the peripheral used
    hw_periph_clock_enable(priv->periph);

    LL_USART_Disable(priv->periph);
    {
        LL_USART_DeInit(priv->periph);
        LL_USART_SetBaudRate(priv->periph, PLAT_APB1_HZ, LL_USART_OVERSAMPLING_16, priv->baudrate);

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
                                      (priv->direction == UUSART_DIRECTION_RX) ? LL_USART_DIRECTION_RX :
                                      (priv->direction == UUSART_DIRECTION_TX) ? LL_USART_DIRECTION_TX
                                                                               : LL_USART_DIRECTION_TX_RX);

        LL_USART_SetHWFlowCtrl(priv->periph,
                               priv->hw_flow_control == 0 ? LL_USART_HWCONTROL_NONE :
                               priv->hw_flow_control == 1 ? LL_USART_HWCONTROL_RTS :
                               priv->hw_flow_control == 2 ? LL_USART_HWCONTROL_CTS
                                                          : LL_USART_HWCONTROL_RTS_CTS);

        LL_USART_ConfigClock(priv->periph,
                             priv->cpha ? LL_USART_PHASE_2EDGE : LL_USART_PHASE_1EDGE,
                             priv->cpol ? LL_USART_POLARITY_HIGH : LL_USART_POLARITY_LOW,
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

        // Prepare for DMA
        LL_USART_ClearFlag_TC(priv->periph);
        LL_USART_EnableDMAReq_RX(priv->periph);
        LL_USART_EnableDMAReq_TX(priv->periph);
    }
    LL_USART_Enable(priv->periph);

    // modifies some usart registers that can't be modified when enabled
    TRY(UUSART_SetupDMAs(unit));

    // timeout based on the baudrate
    unit->tick_interval = (uint16_t) ((50 * 1000) / priv->baudrate); // receive timeout (ms)
    if (unit->tick_interval < 5) unit->tick_interval = 5;

    return E_SUCCESS;
}

/** Tear down the unit */
void UUSART_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init the pins & peripheral only if inited correctly
    if (unit->status == E_SUCCESS) {
        assert_param(priv->periph);
        LL_USART_DeInit(priv->periph);

        // Disable clock
        hw_periph_clock_disable(priv->periph);

        UUSART_DeInitDMAs(unit);
    }

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
    unit->data = NULL;
}
