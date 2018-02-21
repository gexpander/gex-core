//
// Created by MightyPork on 2017/12/08.
//

#ifndef GEX_PLAT_COMPAT_H
#define GEX_PLAT_COMPAT_H

#define VFS_DRIVE_NAME "GEX"

// -------- Priorities -------------
#define TSK_MAIN_PRIO osPriorityNormal
#define TSK_JOBS_PRIO osPriorityHigh
#define TSK_TIMERS_PRIO 4 // this must be in the 0-7 range

// -------- Static buffers ---------
// USB / VFS task stack size
#if DISABLE_MSC
 #define TSK_STACK_MAIN      100 // without MSC the stack usage is significantly lower
#else
 #define TSK_STACK_MAIN      160
#endif

// 180 is normally enough if not doing extensive debug logging
#define TSK_STACK_MSG       200 // TF message handler task stack size (all unit commands run on this thread)
#define TSK_STACK_IDLE    64 //configMINIMAL_STACK_SIZE
#define TSK_STACK_TIMERS  64 //configTIMER_TASK_STACK_DEPTH

#define PLAT_HEAP_SIZE 4096



#define BULK_READ_BUF_LEN 256   // Buffer for TF bulk reads
#define UNIT_TMP_LEN      512   // Buffer for internal unit operations

#define FLASH_SAVE_BUF_LEN  128 // Malloc'd buffer for saving to flash

#define MSG_QUE_SLOT_SIZE 64 // FIXME this should be possible to lower, but there's some bug with bulk transfer / INI parser
#define RX_QUE_CAPACITY    16 // TinyFrame rx queue size (64 bytes each)

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
#define DBG_BUF_LEN      100 // Size of the snprintf buffer for debug messages
#define ERR_MSG_STR_LEN  64 // Error message buffer size
#define IWBUFFER_LEN     80 // Ini writer buffer for sprintf

// -------- Timeouts ------------
#define TF_PARSER_TIMEOUT_TICKS 300 // Timeout for receiving & parsing a frame
#define BULK_LST_TIMEOUT_MS     500 // timeout for the bulk transaction to expire
#define MSG_QUE_POST_TIMEOUT    100 // Time to post to the messages / jobs queue

// -------- Platform specific includes and defines ---------

/// Feature flags:
// PLAT_FLASHBANKS - has the Banks field on the Flash config struct
// PLAT_NO_FLOATING_INPUTS - can't have digital inputs with no pull resistor
// PLAT_USB_PHYCLOCK - requires special config of phy clock for USB
// PLAT_USB_OTGFS - uses the USB OTG IP, needs different config code
// PLAT_LOCK_BTN - use a lock button instead of a lock jumper (push to toggle)
// PLAT_LOCK_1CLOSED - lock jumper is active (closed / button pressed) in logical 1
// PLAT_NO_AFNUM - legacy platform without numbered AF alternatives

#if defined(GEX_PLAT_F103_BLUEPILL)

    // platform name for the version string
    #define GEX_PLATFORM "STM32F103-Bluepill"
    #define PLAT_AHB_MHZ 72
    #define PLAT_APB1_MHZ 36

    // feature flags
    #define PLAT_FLASHBANKS 1
    #define PLAT_NO_FLOATING_INPUTS 1
    #define PLAT_NO_AFNUM 1

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

    // Lock jumper config - re-use BOOT1, closed = LOW
    #define LOCK_JUMPER_PORT 'B'
    #define LOCK_JUMPER_PIN  2

    // Status LED config
    #define STATUS_LED_PORT 'C'
    #define STATUS_LED_PIN  13

#elif defined(GEX_PLAT_F072_DISCOVERY)

    // platform name for the version string
    #define GEX_PLATFORM "STM32F072-Discovery"
    #define PLAT_AHB_MHZ 48
    #define PLAT_APB1_MHZ 48

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
    #define LOCK_JUMPER_PORT 'A'
    #define LOCK_JUMPER_PIN  0
    #define PLAT_LOCK_BTN 1 // toggle button instead of a jumper
    #define PLAT_LOCK_1CLOSED 1 // toggle button active in log. 1

    // Status LED config
    #define STATUS_LED_PORT 'C'
    #define STATUS_LED_PIN  6 // RED LED "UP"

#elif defined(GEX_PLAT_F303_DISCOVERY)

    // platform name for the version string
    #define GEX_PLATFORM "STM32F303-Discovery"
    #define PLAT_AHB_MHZ 72
    #define PLAT_APB1_MHZ 36
    #define PLAT_APB2_MHZ 72

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
    #define LOCK_JUMPER_PORT 'A'
    #define LOCK_JUMPER_PIN  0
    #define PLAT_LOCK_BTN 1 // toggle button instead of a jumper
    #define PLAT_LOCK_1CLOSED 1 // toggle button active in log. 1

    // Status LED config
    #define STATUS_LED_PORT 'E'
    #define STATUS_LED_PIN  13

#elif defined(GEX_PLAT_F407_DISCOVERY)

    // platform name for the version string
    #define GEX_PLATFORM "STM32F407-Discovery"
    #define PLAT_AHB_MHZ 168
    #define PLAT_APB1_MHZ 48
    #define PLAT_APB2_MHZ 96

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
    #define LOCK_JUMPER_PORT 'A'
    #define LOCK_JUMPER_PIN  0
    #define PLAT_LOCK_BTN 1 // toggle button instead of a jumper
    #define PLAT_LOCK_1CLOSED 1 // toggle button active in log. 1

    // Status LED config
    #define STATUS_LED_PORT 'D' // orange
    #define STATUS_LED_PIN  13

#else
    #error "BAD PLATFORM! Please select GEX platform using a -DGEX_PLAT_* compile flag"
#endif

#define PLAT_AHB_HZ (PLAT_AHB_MHZ*1000000)
#define PLAT_APB1_HZ (PLAT_APB1_MHZ*1000000)
#define PLAT_APB2_HZ (PLAT_APB2_MHZ*1000000)

#endif //GEX_PLAT_COMPAT_H
