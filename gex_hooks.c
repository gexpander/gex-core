//
// Created by MightyPork on 2017/12/15.
//

#include "platform.h"
#include "USB/usb_device.h"
#include "TinyFrame.h"
#include "comm/messages.h"
#include "platform/status_led.h"
#include "platform/debug_uart.h"
#include "gex_hooks.h"
#include "unit_registry.h"

/**
 * This is a systick callback for GEX application logic
 */
void GEX_MsTick(void)
{
    TF_Tick(comm);
    Indicator_Tick();
    ureg_tick_units();
}

/**
 * Early init, even before RTOS starts
 */
void GEX_PreInit(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
#if PORTS_COUNT>4
    __HAL_RCC_GPIOE_CLK_ENABLE();
#endif
#if PORTS_COUNT>5
    __HAL_RCC_GPIOF_CLK_ENABLE();
#endif

    Indicator_PreInit();
    DebugUart_PreInit();

    dbg("\r\n\033[37;1m*** GEX "GEX_VERSION" on "GEX_PLATFORM" ***\033[m");
    dbg("Build "__DATE__" "__TIME__);

    PRINTF("Reset cause:");
    if (LL_RCC_IsActiveFlag_LPWRRST()) PRINTF(" LPWR");
    if (LL_RCC_IsActiveFlag_WWDGRST()) PRINTF(" WWDG");
    if (LL_RCC_IsActiveFlag_IWDGRST()) PRINTF(" IWDG");
    if (LL_RCC_IsActiveFlag_SFTRST()) PRINTF(" SFT");
    if (LL_RCC_IsActiveFlag_PORRST()) PRINTF(" POR");
    if (LL_RCC_IsActiveFlag_PINRST()) PRINTF(" PIN");
    if (LL_RCC_IsActiveFlag_OBLRST()) PRINTF(" OBL");
    if (LL_RCC_IsActiveFlag_V18PWRRST()) PRINTF(" V18PWR");
    PUTNL();
    LL_RCC_ClearResetFlags();

    plat_init();

    MX_USB_DEVICE_Init();

    dbg("Starting FreeRTOS...");
}
