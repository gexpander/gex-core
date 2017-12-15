//
// Created by MightyPork on 2017/11/26.
//

#include "platform.h"
#include "usbd_core.h"
#include "USB/usb_device.h"
#include "framework/resources.h"


// ----- SUPPORTED UNITS -----

#include "framework/unit_registry.h"

#include "units/pin/unit_pin.h"
#include "units/neopixel/unit_neopixel.h"

void plat_register_units(void)
{
    ureg_add_type(&UNIT_PIN);
    ureg_add_type(&UNIT_NEOPIXEL);
}


// ----- RELEASE AVAILABLE RESOURCES -----

void plat_init_resources(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    // Platform F103C8T6 - free all present resources
    {
        rsc_free(NULL, R_ADC1);
        rsc_free(NULL, R_ADC2);
        rsc_free(NULL, R_I2C1);
        rsc_free(NULL, R_I2C2);
        rsc_free(NULL, R_SPI1);
        rsc_free(NULL, R_SPI2);
        rsc_free(NULL, R_TIM1);
        rsc_free(NULL, R_TIM2);
        rsc_free(NULL, R_TIM3);
        rsc_free(NULL, R_TIM4);
        rsc_free(NULL, R_USART1);
        rsc_free(NULL, R_USART2);
        rsc_free(NULL, R_USART3);
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
}


// ---- USB reconnect ----

/** USB re-connect */
void plat_usb_reconnect(void)
{
    // F103 doesn't have pull-up control, this is probably the best we can do
    USBD_LL_Reset(&hUsbDeviceFS);
}

