//
// Created by MightyPork on 2017/12/08.
//

#ifndef GEX_PLAT_COMPAT_H
#define GEX_PLAT_COMPAT_H

// platform name for the version string
#define GEX_PLATFORM "STM32F103"

#include <stm32f1xx.h>
#include <stm32f1xx_hal.h>
#include <stm32f1xx_ll_adc.h>
#include <stm32f1xx_ll_bus.h>
#include <stm32f1xx_ll_cortex.h>
#include <stm32f1xx_ll_crc.h>
#include <stm32f1xx_ll_dac.h>
#include <stm32f1xx_ll_dma.h>
#include <stm32f1xx_ll_exti.h>
#include <stm32f1xx_ll_fsmc.h>
#include <stm32f1xx_ll_gpio.h>
#include <stm32f1xx_ll_i2c.h>
#include <stm32f1xx_ll_iwdg.h>
#include <stm32f1xx_ll_pwr.h>
#include <stm32f1xx_ll_rcc.h>
#include <stm32f1xx_ll_rtc.h>
#include <stm32f1xx_ll_sdmmc.h>
#include <stm32f1xx_ll_spi.h>
#include <stm32f1xx_ll_system.h>
#include <stm32f1xx_ll_tim.h>
#include <stm32f1xx_ll_usart.h>
#include <stm32f1xx_ll_usb.h>
#include <stm32f1xx_ll_utils.h>
#include <stm32f1xx_ll_wwdg.h>

// size, determines position of the flash storage
#define FLASH_SIZE (64*1024)
#define SETTINGS_BLOCK_SIZE (1024*2) // this must be a multiple of FLASH pages
#define SETTINGS_FLASH_ADDR (0x08000000 + FLASH_SIZE - SETTINGS_BLOCK_SIZE)

// Number of GPIO ports A,B,C...
#define PORTS_COUNT 5

// Lock jumper config
#define LOCK_JUMPER_PORT 'C'
#define LOCK_JUMPER_PIN  14

// Status LED config
#define STATUS_LED_PORT 'C'
#define STATUS_LED_PIN  13

#endif //GEX_PLAT_COMPAT_H
