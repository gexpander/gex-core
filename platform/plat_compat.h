//
// Created by MightyPork on 2017/12/08.
//

#ifndef GEX_PLAT_COMPAT_H
#define GEX_PLAT_COMPAT_H

#define VFS_DRIVE_NAME "GEX"

// -------- Static buffers ---------
#define TSK_STACK_MAIN      220 // USB / VFS task stack size
#define TSK_STACK_MSG       200 // TF message handler task stack size
#define TSK_STACK_JOBRUNNER 80 // Job runner task stack size

#define BULKREAD_MAX_CHUNK  256 // Bulk read buffer

#define FLASH_SAVE_BUF_LEN  256 // Static buffer for saving to flash

#define JOB_QUEUE_CAPACITY  4 // Job runner queue size (16 bytes each)
#define RX_QUE_CAPACITY    10 // TinyFrame rx queue size (64 bytes each)

#define TF_MAX_PAYLOAD_RX 512 // TF max Rx payload
#define TF_SENDBUF_LEN     64 // TF transmit buffer (can be less than a full frame)

#define TF_MAX_ID_LST   4 // Frame ID listener count
#define TF_MAX_TYPE_LST 6 // Frame Type listener count
#define TF_MAX_GEN_LST  1 // Generic listener count

#define USBD_MAX_STR_DESC_SIZ   64 // Descriptor conversion buffer (used for converting ASCII to UTF-16, must be 2x the size of the longest descriptor)
#define MSC_MEDIA_PACKET       512 // Mass storage sector size (packet)

#define INI_KEY_MAX    20 // Ini parser key buffer
#define INI_VALUE_MAX  30 // Ini parser value buffer

// -------- Stack buffers ----------
#define DBG_BUF_LEN      80 // Size of the snprintf buffer for debug messages
#define ERR_MSG_STR_LEN  64 // Error message buffer size
#define IWBUFFER_LEN     80 // Ini writer buffer for sprintf

// -------- Timeouts ------------
#define TF_PARSER_TIMEOUT_TICKS 100 // Timeout for receiving & parsing a frame
#define BULK_LST_TIMEOUT_MS     200 // timeout for the bulk transaction to expire



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
