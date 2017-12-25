//
// Created by MightyPork on 2017/12/08.
//

#ifndef GEX_PLAT_COMPAT_H
#define GEX_PLAT_COMPAT_H

// -------- Buffers and stack sizes ---------

// FreeRTOS thread stacks (in 4-byte words)
#define TSK_STACK_MAIN      160
#define TSK_STACK_MSG       200
#define TSK_STACK_JOBRUNNER 128

// Size of the snprintf buffer for debug messages
// (this is on stack to avoid races)
#define DBG_BUF_LEN 80

// Bulk read/write
#define BULK_LST_TIMEOUT_MS 200 // timeout for the bulk transaction to expire
#define BULKREAD_MAX_CHUNK  512 // this is a static buffer

// Error message buffer size (on stack)
#define ERR_MSG_STR_LEN 32

// Static buffer for saving to flash
#define FLASH_SAVE_BUF_LEN 256

// Number of job runner slots
#define HP_SCHED_CAPACITY 5

// Number of message queue slots (64 bytes each)
#define RX_QUE_CAPACITY 10


// ------ TinyFrame config ------

// Maximum received payload size (static buffer)
// Larger payloads will be rejected.
#define TF_MAX_PAYLOAD_RX 640

// Size of the sending buffer. Larger payloads will be split to pieces and sent
// in multiple calls to the write function. This can be lowered to reduce RAM usage.
#define TF_SENDBUF_LEN 64

// --- Listener counts - determine sizes of the static slot tables ---

// Frame ID listeners (wait for response / multi-part message)
#define TF_MAX_ID_LST   4
// Frame Type listeners (wait for frame with a specific first payload byte)
#define TF_MAX_TYPE_LST 6
// Generic listeners (fallback if no other listener catches it)
#define TF_MAX_GEN_LST  1

// Timeout for receiving & parsing a frame
// ticks = number of calls to TF_Tick()
#define TF_PARSER_TIMEOUT_TICKS 250


// --- Mass Storage / USB config ---

#define USBD_MAX_STR_DESC_SIZ  128
#define MSC_MEDIA_PACKET       512


// INI buffer sizes
#define INI_KEY_MAX 20
#define INI_VALUE_MAX 30
// ini writer
#define IWBUFFER_LEN 128



// -------- Platform specific includes and defines ---------

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

#elif defined(GEX_PLAT_F303_DISCOVERY)

    // platform name for the version string
    #define GEX_PLATFORM "STM32F303-Discovery"

    #include <stm32f3xx.h>
    #include <stm32f3xx_hal.h>
    #include <stm32f3xx_ll_adc.h>
    #include <stm32f3xx_ll_bus.h>
    #include <stm32f3xx_ll_comp.h>
    #include <stm32f3xx_ll_cortex.h>
    #include <stm32f3xx_ll_crc.h>
    #include <stm32f3xx_ll_dac.h>
    #include <stm32f3xx_ll_dma.h>
    #include <stm32f3xx_ll_exti.h>
    #include <stm32f3xx_ll_fmc.h>
    #include <stm32f3xx_ll_gpio.h>
    #include <stm32f3xx_ll_hrtim.h>
    #include <stm32f3xx_ll_i2c.h>
    #include <stm32f3xx_ll_iwdg.h>
    #include <stm32f3xx_ll_opamp.h>
    #include <stm32f3xx_ll_pwr.h>
    #include <stm32f3xx_ll_rcc.h>
    #include <stm32f3xx_ll_rtc.h>
    #include <stm32f3xx_ll_spi.h>
    #include <stm32f3xx_ll_system.h>
    #include <stm32f3xx_ll_tim.h>
    #include <stm32f3xx_ll_usart.h>
    #include <stm32f3xx_ll_utils.h>
    #include <stm32f3xx_ll_wwdg.h>

    // size, determines position of the flash storage
    #define FLASH_SIZE (256*1024)
    #define SETTINGS_BLOCK_SIZE (1024*2) // this must be a multiple of FLASH pages
    #define SETTINGS_FLASH_ADDR (0x08000000 + FLASH_SIZE - SETTINGS_BLOCK_SIZE)

    // Number of GPIO ports A,B,C...
    #define PORTS_COUNT 6

    // Lock jumper config
    #define LOCK_JUMPER_PORT 'F'
    #define LOCK_JUMPER_PIN  1 // OSC OUT, not used in BYPASS mode (receiving clock from the ST-Link MCO)

    // Status LED config
    #define STATUS_LED_PORT 'E'
    #define STATUS_LED_PIN  13

#elif defined(GEX_PLAT_F407_DISCOVERY)

// platform name for the version string
    #define GEX_PLATFORM "STM32F407-Discovery"

    #define PLAT_USB_PHYCLOCK 1
    #define PLAT_USB_OTGFS 1

    #include <stm32f4xx.h>
    #include <stm32f4xx_hal.h>
    #include <stm32f4xx_ll_adc.h>
    #include <stm32f4xx_ll_crc.h>
    #include <stm32f4xx_ll_dac.h>
    #include <stm32f4xx_ll_dma.h>
    #include <stm32f4xx_ll_dma2d.h>
    #include <stm32f4xx_ll_exti.h>
    #include <stm32f4xx_ll_fmc.h>
    #include <stm32f4xx_ll_fsmc.h>
    #include <stm32f4xx_ll_gpio.h>
    #include <stm32f4xx_ll_i2c.h>
    #include <stm32f4xx_ll_lptim.h>
    #include <stm32f4xx_ll_pwr.h>
    #include <stm32f4xx_ll_rcc.h>
    #include <stm32f4xx_ll_rng.h>
    #include <stm32f4xx_ll_rtc.h>
    #include <stm32f4xx_ll_sdmmc.h>
    #include <stm32f4xx_ll_spi.h>
    #include <stm32f4xx_ll_tim.h>
    #include <stm32f4xx_ll_usart.h>
    #include <stm32f4xx_ll_usb.h>
    #include <stm32f4xx_ll_utils.h>

    // size, determines position of the flash storage

    // we use the first 128kB sector. Unfortunately the whole sector must be erased before writing.
    #define SETTINGS_FLASH_SECTOR FLASH_SECTOR_5
    #define SETTINGS_BLOCK_SIZE (1024*2)
    #define SETTINGS_FLASH_ADDR (0x08000000 + (16*4+64)*1024)

    // Number of GPIO ports A,B,C...
    #define PORTS_COUNT 6

    // Lock jumper config
    #define LOCK_JUMPER_PORT 'C'
    #define LOCK_JUMPER_PIN  14 // completely random choice here... TBD

    // Status LED config
    #define STATUS_LED_PORT 'D' // orange
    #define STATUS_LED_PIN  13

#else
    #error "BAD PLATFORM! Please select GEX platform using a -DGEX_PLAT_* compile flag"
#endif

#endif //GEX_PLAT_COMPAT_H
