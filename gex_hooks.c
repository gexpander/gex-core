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

/**
 * This is a systick callback for GEX application logic
 */
void GEX_MsTick(void)
{
    TF_Tick(comm);
    StatusLed_Tick();
}

/**
 * Early init, even before RTOS starts
 */
void GEX_PreInit(void)
{
    DebugUart_PreInit();
    dbg("\r\n\033[37;1m*** GEX "GEX_VERSION" on "GEX_PLATFORM" ***\033[m");
    dbg("Build "__DATE__" "__TIME__"\r\n");

    plat_init();

    MX_USB_DEVICE_Init();

    dbg("Starting FreeRTOS...");
}
