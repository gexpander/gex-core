//
// Created by MightyPork on 2017/11/26.
//

#include "platform.h"
#include "usbd_core.h"
#include "USB/usb_device.h"
#include "framework/resources.h"
#include "framework/unit_registry.h"
#include "units/digital_out/unit_dout.h"

#include "units/digital_in/unit_din.h"
#include "units/neopixel/unit_neopixel.h"
#include "units/i2c/unit_i2c.h"
#include "units/1wire/unit_1wire.h"
#include "units/adc/unit_adc.h"
#include "units/test/unit_test.h"
#include "units/usart/unit_usart.h"
#include "units/spi/unit_spi.h"
#include "units/sipo/unit_sipo.h"
#include "units/fcap/unit_fcap.h"
#include "units/touch/unit_touch.h"
#include "units/simple_pwm/unit_pwmdim.h"
#include "units/dac/unit_dac.h"
#include "comm/interfaces.h"
#include "hw_utils.h"

void plat_init_resources(void)
{
    // we use |= here, even though return values are not booleans - success is 0,
    // thus any problem results in nonzero and the assert will trip.
    uint32_t rv = 0;

    // periphs available everywhere - enable clock
    hw_periph_clock_enable(DMA1);
#ifdef DMA2
    hw_periph_clock_enable(DMA2);
#endif

    // --- Common unit drivers ---

    #if HAVE_TEST_UNIT
        ureg_add_type(&UNIT_TEST);
    #endif

    // EXTI are always available
    rsc_free_range(NULL, R_EXTI0, R_EXTI15);

    // --- platform specific resource releases and claims ---

#if defined(GEX_PLAT_F103_BLUEPILL)
    // Platform STM32F103C8T6 Bluepill ($4 board from eBay)

    // Units supported by the platform (known to work correctly)

    // free all present resources
    {
        rsc_free_range(NULL, R_ADC1, R_ADC2);
        rsc_free_range(NULL, R_I2C1, R_I2C2);
        rsc_free_range(NULL, R_SPI1, R_SPI2);
        rsc_free_range(NULL, R_TIM1, R_TIM4);
        rsc_free_range(NULL, R_USART1, R_USART3);
        rsc_free_range(NULL, R_PA0, R_PA15);
        rsc_free_range(NULL, R_PB0, R_PB15);
        rsc_free_range(NULL, R_PC13, R_PC15);
        rsc_free_range(NULL, R_PD0, R_PD1);
    }

    // Claim resources not available due to board layout or internal usage
    {
        // HAL timebase
        rv |= rsc_claim(&UNIT_SYSTEM, R_TIM1);
        // HSE crystal
        rv |= rsc_claim(&UNIT_SYSTEM, R_PD0);
        rv |= rsc_claim(&UNIT_SYSTEM, R_PD1);
        // SWD
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA13);
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA14);
        // USB
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA11);
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA12);
        // BOOT pin(s)
        rv |= rsc_claim(&UNIT_SYSTEM, R_PB2); // BOOT1
    }
#elif defined(STM32F072xB)
    // Platform STM32F073RBT

    // Additional GPIO ports
    __HAL_RCC_GPIOF_CLK_ENABLE();

    // Units supported by the platform (known to work correctly)
    ureg_add_type(&UNIT_DOUT);
    ureg_add_type(&UNIT_DIN);
    ureg_add_type(&UNIT_NEOPIXEL);
    ureg_add_type(&UNIT_I2C);
    ureg_add_type(&UNIT_SPI);
    ureg_add_type(&UNIT_USART);
    ureg_add_type(&UNIT_1WIRE);
    ureg_add_type(&UNIT_ADC);
    ureg_add_type(&UNIT_SIPO);
    ureg_add_type(&UNIT_FCAP);
    ureg_add_type(&UNIT_TOUCH);
    ureg_add_type(&UNIT_PWMDIM);
    ureg_add_type(&UNIT_DAC);

    // Free all present resources
    {
        rsc_free(NULL, R_ADC1);
//        rsc_free(NULL, R_CAN1);
//        rsc_free_range(NULL, R_COMP1, R_COMP2);
        rsc_free(NULL, R_DAC1);
//        rsc_free(NULL, R_HDMI_CEC);
        rsc_free(NULL, R_TSC);
        rsc_free_range(NULL, R_I2C1, R_I2C2);
//        rsc_free_range(NULL, R_I2S1, R_I2S2);
        rsc_free_range(NULL, R_SPI1, R_SPI2);
        rsc_free_range(NULL, R_TIM1, R_TIM3);
        rsc_free_range(NULL, R_TIM6, R_TIM7);
        rsc_free_range(NULL, R_TIM14, R_TIM17);
        rsc_free_range(NULL, R_USART1, R_USART4);
        rsc_free_range(NULL, R_DMA1_1, R_DMA1_7);

        rsc_free_range(NULL, R_PA0, R_PA15);
        rsc_free_range(NULL, R_PB0, R_PB15);
        rsc_free_range(NULL, R_PC0, R_PC15);
        rsc_free(NULL, R_PD2);
        rsc_free_range(NULL, R_PF0, R_PF1);
    }

    // Claim resources not available due to board layout or internal usage
    {
        // HAL timebase
        rv |= rsc_claim(&UNIT_SYSTEM, R_TIM17);
        // HSE crystal
        rv |= rsc_claim(&UNIT_SYSTEM, R_PF0);

        #if PLAT_FULL_XTAL
            rv |= rsc_claim(&UNIT_SYSTEM, R_PF1); // - not used in BYPASS mode
        #endif

        // SWD
//        rv |= rsc_claim(&UNIT_SYSTEM, R_PA13);
//        rv |= rsc_claim(&UNIT_SYSTEM, R_PA14);
        // USB
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA11);
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA12);

        #if defined(GEX_PLAT_F072_ZERO)
            // unconnected pins
            rv |= rsc_claim_range(&UNIT_PLATFORM, R_PC0, R_PC1);
            rv |= rsc_claim_range(&UNIT_PLATFORM, R_PC4, R_PC9);
        #endif
    }
#elif defined(GEX_PLAT_F303_DISCOVERY)
    // Platform STM32F303VCT

    // Additional GPIO ports
    __HAL_RCC_GPIOF_CLK_ENABLE();

    // Units supported by the platform (known to work correctly)
    // ureg_add_type(&UNIT_XYZ);

    // Free all present resources
    {
        rsc_free_range(NULL, R_ADC1, R_ADC4);
//        rsc_free(NULL, R_CAN1);
//        rsc_free_range(NULL, R_COMP1, R_COMP7);
//        rsc_free(NULL, R_HDMI_CEC);
        rsc_free(NULL, R_DAC1);
        rsc_free_range(NULL, R_I2C1, R_I2C2);
        rsc_free_range(NULL, R_I2S2, R_I2S3);
//        rsc_free_range(NULL, R_OPAMP1, R_OPAMP4);
        rsc_free_range(NULL, R_SPI1, R_SPI3);
        rsc_free_range(NULL, R_TIM1, R_TIM4);
        rsc_free_range(NULL, R_TIM6, R_TIM8);
        rsc_free_range(NULL, R_TIM15, R_TIM17);
        rsc_free(NULL, R_TSC);
        rsc_free_range(NULL, R_USART1, R_USART5);

        rsc_free_range(NULL, R_PA0, R_PA15);
        rsc_free_range(NULL, R_PB0, R_PB15);
        rsc_free_range(NULL, R_PC0, R_PC15);
        rsc_free_range(NULL, R_PD0, R_PD15);
        rsc_free_range(NULL, R_PE0, R_PE15);

        rsc_free_range(NULL, R_PF0, R_PF2);
        rsc_free(NULL, R_PF4);
        rsc_free_range(NULL, R_PF9, R_PF10);
    }

    // Claim resources not available due to board layout or internal usage
    {
        // HAL timebase
        rv |= rsc_claim(&UNIT_SYSTEM, R_TIM1);
        // HSE crystal
        rv |= rsc_claim(&UNIT_SYSTEM, R_PF0);
        //rv |= rsc_claim(&UNIT_SYSTEM, R_PF1); // - not used in BYPASS mode
        // SWD
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA13);
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA14);
        // USB
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA11);
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA12);
        // BOOT pin(s)
        rv |= rsc_claim(&UNIT_SYSTEM, R_PB2); // BOOT1
    }
#elif defined(GEX_PLAT_F407_DISCOVERY)
    // Platform STM32F407VGT

    // Additional GPIO ports
    __HAL_RCC_GPIOF_CLK_ENABLE();

    // Units supported by the platform (known to work correctly)
    // ureg_add_type(&UNIT_XYZ);

    // Free all present resources
    {
        rsc_free_range(NULL, R_ADC1, R_ADC3);
//        rsc_free_range(NULL, R_CAN1, R_CAN2);
//        rsc_free_range(NULL, R_COMP1, R_COMP7);
        rsc_free(NULL, R_DAC1);
//        rsc_free(NULL, R_DCMI);
//        rsc_free(NULL, R_ETH);
//        rsc_free(NULL, R_FSMC);
        rsc_free_range(NULL, R_I2C1, R_I2C3);
        rsc_free_range(NULL, R_I2S2, R_I2S3);
//        rsc_free(NULL, R_SDIO);
        rsc_free_range(NULL, R_SPI1, R_SPI3);
        rsc_free_range(NULL, R_TIM1, R_TIM14);
        rsc_free_range(NULL, R_USART1, R_USART3);
        rsc_free(NULL, R_USART6);

        rsc_free_range(NULL, R_PA0, R_PA15);
        rsc_free_range(NULL, R_PB0, R_PB15);
        rsc_free_range(NULL, R_PC0, R_PC15);
        rsc_free_range(NULL, R_PD0, R_PD15);
        rsc_free_range(NULL, R_PE0, R_PE15);
        // also has 2 PH pins

        // F407 appears to have fewer GPIOs than F303?
    }

    // Claim resources not available due to board layout or internal usage
    {
        // HAL timebase
        rv |= rsc_claim(&UNIT_SYSTEM, R_TIM1);
        // HSE crystal
        // H0 and H1
        // SWD
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA13);
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA14);
        // USB
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA11);
        rv |= rsc_claim(&UNIT_SYSTEM, R_PA12);
        // BOOT pin(s)
        rv |= rsc_claim(&UNIT_SYSTEM, R_PB2); // BOOT1
    }
#else
    #error "BAD PLATFORM!"
#endif

    assert_param(rv == 0);
}


// ---- USB reconnect ----

/**
 * USB re-connect (to apply change of the LOCK jumper)
 */
void plat_usb_reconnect(void)
{
    if (gActiveComport != COMPORT_USB) return;

    // TODO add better reset methods available on different chips

    USBD_LL_Reset(&hUsbDeviceFS);

#if defined(GEX_PLAT_F103_BLUEPILL)
    // F103 doesn't have pull-up control
#else
    HAL_PCD_DevDisconnect(&hpcd_USB_FS);
    osDelay(100);
    HAL_PCD_DevConnect(&hpcd_USB_FS);
#endif

}

void plat_print_system_pinout(IniWriter *iw)
{
    if (iw->count == 0) return;

    iw_string(iw, "System pin-out\r\n"
                  "--------------\r\n");

    #if PLAT_LOCK_BTN
        iw_sprintf(iw, "Lock button (active="
                #if PLAT_LOCK_1CLOSED
                    "high"
                #else
                    "low"
                #endif
            "): P%c%d\r\n", LOCK_JUMPER_PORT, LOCK_JUMPER_PIN);
    #else
        iw_sprintf(iw, "Lock jumper (closed="
                #if PLAT_LOCK_1CLOSED
                    "high"
                #else
                    "low"
                #endif
            "): P%c%d\r\n", LOCK_JUMPER_PORT, LOCK_JUMPER_PIN);
    #endif

    iw_sprintf(iw, "Indicator LED (anode): P%c%d\r\n", STATUS_LED_PORT, STATUS_LED_PIN);
    iw_sprintf(iw, "System clock speed: %d MHz\r\n", PLAT_AHB_MHZ);
}
