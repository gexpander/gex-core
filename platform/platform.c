//
// Created by MightyPork on 2017/11/26.
//

#include "platform.h"
#include "usbd_core.h"
#include "USB/usb_device.h"
#include "framework/resources.h"
#include "framework/unit_registry.h"

#include "units/pin/unit_pin.h"
#include "units/neopixel/unit_neopixel.h"
#include "units/test/unit_test.h"


// ----- SUPPORTED UNITS -----

void plat_register_units(void)
{
    ureg_add_type(&UNIT_PIN);
    ureg_add_type(&UNIT_NEOPIXEL);

    ureg_add_type(&UNIT_TEST);

    // Platform-specific units could be added here
}


// ----- RELEASE AVAILABLE RESOURCES -----

void plat_init_resources(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

#if defined(GEX_PLAT_F103_BLUEPILL)

    // Platform F103C8T6 - free all present resources
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
        bool ok = true;

        // HAL timebase
        ok &= rsc_claim(&UNIT_SYSTEM, R_TIM1);
        // HSE crystal
        ok &= rsc_claim(&UNIT_SYSTEM, R_PD0);
        ok &= rsc_claim(&UNIT_SYSTEM, R_PD1);
        // SWD
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA13);
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA14);
        // USB
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA11);
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA12);
        // BOOT pin(s)
        ok &= rsc_claim(&UNIT_SYSTEM, R_PB2); // BOOT1

        assert_param(ok);
    }
#elif defined(GEX_PLAT_F072_DISCOVERY)
    __HAL_RCC_GPIOF_CLK_ENABLE();

    // Platform F073RBT - free all present resources
    {
        rsc_free(NULL, R_ADC1);
        rsc_free(NULL, R_CAN1);
        rsc_free_range(NULL, R_COMP1, R_COMP2);
        rsc_free(NULL, R_DAC1);
        rsc_free(NULL, R_HDMI_CEC);
        rsc_free(NULL, R_TSC);
        rsc_free_range(NULL, R_I2C1, R_I2C2);
        rsc_free_range(NULL, R_I2S1, R_I2S2);
        rsc_free_range(NULL, R_SPI1, R_SPI2);
        rsc_free_range(NULL, R_TIM1, R_TIM3);
        rsc_free_range(NULL, R_TIM6, R_TIM7);
        rsc_free_range(NULL, R_TIM14, R_TIM17);
        rsc_free_range(NULL, R_USART1, R_USART4);

        rsc_free_range(NULL, R_PA0, R_PA15);
        rsc_free_range(NULL, R_PB0, R_PB15);
        rsc_free_range(NULL, R_PC0, R_PC15);
        rsc_free(NULL, R_PD2);
        rsc_free_range(NULL, R_PF0, R_PF1);
    }

    // Claim resources not available due to board layout or internal usage
    {
        bool ok = true;

        // HAL timebase
        ok &= rsc_claim(&UNIT_SYSTEM, R_TIM1);
        // HSE crystal
        ok &= rsc_claim(&UNIT_SYSTEM, R_PF0);
        //ok &= rsc_claim(&UNIT_SYSTEM, R_PF1); // - not used in BYPASS mode
        // SWD
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA13);
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA14);
        // USB
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA11);
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA12);
        // BOOT pin(s)
        ok &= rsc_claim(&UNIT_SYSTEM, R_PB2); // BOOT1

        assert_param(ok);
    }
#elif defined(GEX_PLAT_F303_DISCOVERY)
    __HAL_RCC_GPIOF_CLK_ENABLE();

    // Platform F303VCT - free all present resources
    {
        rsc_free_range(NULL, R_ADC1, R_ADC4);
        rsc_free(NULL, R_CAN1);
        rsc_free_range(NULL, R_COMP1, R_COMP7);
        rsc_free(NULL, R_HDMI_CEC);
        rsc_free(NULL, R_DAC1);
        rsc_free_range(NULL, R_I2C1, R_I2C2);
        rsc_free_range(NULL, R_I2S2, R_I2S3);
        rsc_free_range(NULL, R_OPAMP1, R_OPAMP4);
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
        bool ok = true;

        // HAL timebase
        ok &= rsc_claim(&UNIT_SYSTEM, R_TIM1);
        // HSE crystal
        ok &= rsc_claim(&UNIT_SYSTEM, R_PF0);
        //ok &= rsc_claim(&UNIT_SYSTEM, R_PF1); // - not used in BYPASS mode
        // SWD
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA13);
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA14);
        // USB
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA11);
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA12);
        // BOOT pin(s)
        ok &= rsc_claim(&UNIT_SYSTEM, R_PB2); // BOOT1

        assert_param(ok);
    }
#elif defined(GEX_PLAT_F407_DISCOVERY)
    __HAL_RCC_GPIOF_CLK_ENABLE();

    // Platform F407VGT - free all present resources
    {
        rsc_free_range(NULL, R_ADC1, R_ADC3);
        rsc_free_range(NULL, R_CAN1, R_CAN2);
        rsc_free_range(NULL, R_COMP1, R_COMP7);
        rsc_free(NULL, R_DAC1);
        rsc_free(NULL, R_DCMI);
        rsc_free(NULL, R_ETH);
        rsc_free(NULL, R_FSMC);
        rsc_free_range(NULL, R_I2C1, R_I2C3);
        rsc_free_range(NULL, R_I2S2, R_I2S3);
        rsc_free(NULL, R_SDIO);
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
        bool ok = true;

        // HAL timebase
        ok &= rsc_claim(&UNIT_SYSTEM, R_TIM1);
        // HSE crystal
        // H0 and H1
        // SWD
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA13);
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA14);
        // USB
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA11);
        ok &= rsc_claim(&UNIT_SYSTEM, R_PA12);
        // BOOT pin(s)
        ok &= rsc_claim(&UNIT_SYSTEM, R_PB2); // BOOT1

        assert_param(ok);
    }
#else
    #error "BAD PLATFORM!"
#endif
}


// ---- USB reconnect ----

/**
 * USB re-connect (to apply change of the LOCK jumper)
 */
void plat_usb_reconnect(void)
{
    // TODO add better reset methods available on different chips

    // F103 doesn't have pull-up control, this is probably the best we can do
    // This does not seem to trigger descriptors reload.
    USBD_LL_Reset(&hUsbDeviceFS);
}

