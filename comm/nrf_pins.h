//
// Created by MightyPork on 2018/04/06.
//

#ifndef GEX_F072_NRF_PINS_H
#define GEX_F072_NRF_PINS_H

#include "platform.h"

// TODO move those to plat_compat?
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

#define NRF_R_SCK          R_PA5
#define NRF_R_MISO         R_PA6
#define NRF_R_MOSI         R_PA7
#define NRF_SPI_AF         LL_GPIO_AF_0

#endif //GEX_F072_NRF_PINS_H
