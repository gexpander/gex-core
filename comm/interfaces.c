//
// Created by MightyPork on 2018/03/23.
//

#include "platform.h"
#include "usbd_core.h"
#include "USB/usb_device.h"
#include "interfaces.h"
#include "framework/system_settings.h"
#include "framework/unit.h"
#include "framework/resources.h"
#include "platform/hw_utils.h"
#include "framework/unit_base.h"
#include "nrf_pins.h"
#include "iface_uart.h"
#include "iface_usb.h"
#include "iface_nordic.h"

const char * COMPORT_NAMES[] = {
    "NONE",
    "USB",
    "UART",
    "NRF",
    "LORA",
};


enum ComportSelection gActiveComport = COMPORT_USB; // start with USB so the handlers work correctly initially

static uint32_t last_switch_time = 0; // started with USB
static bool xfer_verify_done = false;

static bool configure_interface(enum ComportSelection iface);

/** Switch com transfer if the current one doesnt seem to work */
void com_switch_transfer_if_needed(void)
{
    if (xfer_verify_done) return;

    const uint32_t now = HAL_GetTick();
    const uint32_t elapsed = now - last_switch_time;

    if (gActiveComport == COMPORT_USB) {
        if (elapsed > 1000) {
            // USB may or may not work, depending on whether the module is plugged in
            // or running from a battery/external supply remotely.

            // Check if USB is enumerated

            if (!iface_usb_ready()) {
                dbg("Not enumerated, assuming USB is dead");

                // Fallback to radio or bare USART
                do {
                    if (SystemSettings.use_comm_nordic) {
                        if (configure_interface(COMPORT_NORDIC)) break;
                    }

#if 0
                    if (SystemSettings.use_comm_lora) {
                        if (configure_interface(COMPORT_LORA)) break;
                    }
#endif

                    if (SystemSettings.use_comm_uart) {
                        // after nordic/lora
                        if (configure_interface(COMPORT_USART)) break;
                    }

                    dbg("No alternate com interface configured.");
                    gActiveComport = COMPORT_NONE;
                } while (0);
            } else {
                dbg("USB got address - OK");
            }

            xfer_verify_done = true;
        }
    }
}

/** Claim resources that may be needed for alternate transfers */
void com_claim_resources_for_alt_transfers(void)
{
    if (SystemSettings.use_comm_uart) {
        iface_uart_claim_resources();
    }

    if (SystemSettings.use_comm_nordic) {
        iface_nordic_claim_resources();
    }
}

/** Release resources allocated for alternate transfers */
void com_release_resources_for_alt_transfers(void)
{
    if (SystemSettings.use_comm_uart) {
        iface_uart_free_resources();
    }

    if (SystemSettings.use_comm_nordic) {
        iface_nordic_free_resources();
    }
}


static bool configure_interface(enum ComportSelection iface)
{
    // Teardown
    if (gActiveComport == COMPORT_USB) {
        // simplest USB disabling (XXX needs porting)
        HAL_PCD_DeInit(&hpcd_USB_FS);
        __HAL_RCC_USB_CLK_DISABLE();
    }
    else if (gActiveComport == COMPORT_USART) {
        iface_uart_deinit();
    }
    else if (gActiveComport == COMPORT_NORDIC) {
        iface_nordic_deinit();
    }


    gActiveComport = iface;

    // Init
    if (iface == COMPORT_USB) {
        trap("illegal"); // this never happens
        return false;
    }
    else if (iface == COMPORT_USART) {
        return iface_uart_init();
    }
    else if (iface == COMPORT_NORDIC) {
        return iface_nordic_init();
    }
#if 0
    else if (iface == COMPORT_LORA) {
        // Try to configure nordic
        dbg("Setting up LoRa transfer");
        // TODO set up and check LoRa transport
        dbg("LoRa not impl!");
        return false;
    }
#endif
    else {
        trap("Bad iface %d", iface);
    }
}
