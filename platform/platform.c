//
// Created by MightyPork on 2017/11/26.
//

#include "platform.h"
#include "usbd_core.h"
#include "USB/usb_device.h"
#include "framework/resources.h"
#include "framework/unit_registry.h"
#include "comm/interfaces.h"
#include "hw_utils.h"

#include "units_manifest.h"

extern uint32_t plat_init_resources2(void);

// TODO split this and the plat_compat files to per-platform ones stored in the platform project

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

    // EXTI are always available
    rsc_free_range(NULL, R_EXTI0, R_EXTI15);

    // --- platform specific resource releases and claims ---

    // Units supported by the platform (known to work correctly)
    // - this is a macro created in the Makefile, registering all enabled units
    UNITS_REGISTER_CMD;

    rv = plat_init_resources2();
    assert_param(rv == 0);
}


// ---- USB reconnect ----

/**
 * USB re-connect (to apply change of the LOCK jumper)
 */
void plat_usb_reconnect(void)
{
    if (gActiveComport != COMPORT_USB) return;

    // TODO add other reset methods available on different chips (e.g. externam FET)
    USBD_LL_Reset(&hUsbDeviceFS);

#if PLAT_USB_PU_CTL
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
