//
// Created by MightyPork on 2017/12/08.
//

#ifndef GEX_PLAT_COMPAT_H
#define GEX_PLAT_COMPAT_H

#if defined(GEX_PLAT_F103_BLUEPILL)

    // platform name for the version string
    #define GEX_PLATFORM "STM32F103-Bluepill"

    // This platform has the Banks field on the Flash config struct for settings save
    #define PLAT_FLASHBANKS 1

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

#elif defined(GEX_PLAT_F072_DISCOVERY)

    // platform name for the version string
    #define GEX_PLATFORM "STM32F072-Discovery"

    #include <stm32f0xx.h>
    #include <stm32f0xx_ll_adc.h>
    #include <stm32f0xx_ll_bus.h>
    #include <stm32f0xx_ll_comp.h>
    #include <stm32f0xx_ll_cortex.h>
    #include <stm32f0xx_ll_crc.h>
    #include <stm32f0xx_ll_crs.h>
    #include <stm32f0xx_ll_dac.h>
    #include <stm32f0xx_ll_dma.h>
    #include <stm32f0xx_ll_exti.h>
    #include <stm32f0xx_ll_gpio.h>
    #include <stm32f0xx_ll_i2c.h>
    #include <stm32f0xx_ll_iwdg.h>
    #include <stm32f0xx_ll_pwr.h>
    #include <stm32f0xx_ll_rcc.h>
    #include <stm32f0xx_ll_rtc.h>
    #include <stm32f0xx_ll_spi.h>
    #include <stm32f0xx_ll_system.h>
    #include <stm32f0xx_ll_tim.h>
    #include <stm32f0xx_ll_usart.h>
    #include <stm32f0xx_ll_utils.h>
    #include <stm32f0xx_ll_wwdg.h>

    // size, determines position of the flash storage
    #define FLASH_SIZE (128*1024)
    #define SETTINGS_BLOCK_SIZE (1024*2) // this must be a multiple of FLASH pages
    #define SETTINGS_FLASH_ADDR (0x08000000 + FLASH_SIZE - SETTINGS_BLOCK_SIZE)

    // Number of GPIO ports A,B,C...
    #define PORTS_COUNT 6

    // Lock jumper config
    #define LOCK_JUMPER_PORT 'F'
    #define LOCK_JUMPER_PIN  1 // OSC OUT, not used in BYPASS mode (receiving clock from the ST-Link MCO)

    // Status LED config
    #define STATUS_LED_PORT 'C'
    #define STATUS_LED_PIN  6 // RED LED "UP"
#else
    #error "BAD PLATFORM! Please select GEX platform using a -DGEX_PLAT_* compile flag"
#endif

#endif //GEX_PLAT_COMPAT_H
