//
// Created by MightyPork on 2018/04/06.
//

#ifndef GEX_F072_NRF_PINS_H
#define GEX_F072_NRF_PINS_H

#include "platform.h"

#if defined(GEX_PLAT_F072_DISCOVERY) || defined(GEX_PLAT_F072_HUB)

    // This config was used only for development when NRF was enabled for those platforms. It is normally disabled.
    #define NRF_SPI            SPI1
    #define NRF_R_SPI          R_SPI1

    #define NRF_IRQ_Pin        LL_GPIO_PIN_10
    #define NRF_IRQ_GPIO_Port  GPIOB
    #define NRF_R_IRQ          R_PB10
    #define NRF_EXTI_LINENUM   10
    #define NRF_SYSCFG_EXTI_PORT LL_SYSCFG_EXTI_PORTB

    #define NRF_NSS_Pin        LL_GPIO_PIN_11
    #define NRF_NSS_GPIO_Port  GPIOB
    #define NRF_R_NSS          R_PB11

    #define NRF_CE_Pin         LL_GPIO_PIN_12
    #define NRF_CE_GPIO_Port   GPIOB
    #define NRF_R_CE           R_PB12

    #define NRF_RST_Pin        LL_GPIO_PIN_13
    #define NRF_RST_GPIO_Port  GPIOB
    #define NRF_R_RST          R_PB13

    #define NRF_R_SCK          R_PA5
    #define NRF_SCK_AF         LL_GPIO_AF_0
    #define NRF_R_MISO         R_PA6
    #define NRF_MISO_AF        LL_GPIO_AF_0
    #define NRF_R_MOSI         R_PA7
    #define NRF_MOSI_AF        LL_GPIO_AF_0

#elif defined(GEX_PLAT_F072_ZERO)

    #define NRF_SPI            SPI2
    #define NRF_R_SPI          R_SPI2

    #define NRF_IRQ_Pin        LL_GPIO_PIN_15
    #define NRF_IRQ_GPIO_Port  GPIOC
    #define NRF_R_IRQ          R_PC15
    #define NRF_EXTI_LINENUM   15
    #define NRF_SYSCFG_EXTI_PORT LL_SYSCFG_EXTI_PORTC

    #define NRF_NSS_Pin        LL_GPIO_PIN_13
    #define NRF_NSS_GPIO_Port  GPIOC
    #define NRF_R_NSS          R_PC13

    #define NRF_CE_Pin         LL_GPIO_PIN_14
    #define NRF_CE_GPIO_Port   GPIOC
    #define NRF_R_CE           R_PC14

    #define NRF_RST_Pin        LL_GPIO_PIN_12
    #define NRF_RST_GPIO_Port  GPIOC
    #define NRF_R_RST          R_PC12

    #define NRF_R_SCK          R_PB13
    #define NRF_SCK_AF         LL_GPIO_AF_0
    #define NRF_R_MISO         R_PC2
    #define NRF_MISO_AF        LL_GPIO_AF_1
    #define NRF_R_MOSI         R_PC3
    #define NRF_MOSI_AF        LL_GPIO_AF_1

#else
    #error "Missing NRF config for this platform."
#endif

#endif //GEX_F072_NRF_PINS_H
