//
// Created by MightyPork on 2018/03/23.
//

#include <stm32f072xb.h>
#include "platform.h"
#include "usbd_core.h"
#include "USB/usb_device.h"
#include "interfaces.h"
#include "framework/system_settings.h"
#include "framework/unit.h"
#include "framework/resources.h"
#include "platform/hw_utils.h"
#include "framework/unit_base.h"

enum ComportSelection gActiveComport = COMPORT_USB; // start with USB so the handlers work correctly initially

static uint32_t last_switch_time = 0; // started with USB
static bool xfer_verify_done = false;

static void configure_interface(enum ComportSelection iface);

/** Switch com transfer if the current one doesnt seem to work */
void com_switch_transfer_if_needed(void)
{
    if (xfer_verify_done) return;

    const uint32_t now = HAL_GetTick();
    const uint32_t elapsed = now - last_switch_time;

    if (gActiveComport == COMPORT_USB) {
        if (elapsed > 1000) {
            // USB may or may not work, depending on whether the module is plugged -
            // in or running from a battery/external supply remotely.

            // Check if USB is enumerated

            const uint32_t uadr = (USB->DADDR & USB_DADDR_ADD);
            if (0 == uadr) {
                dbg("Not enumerated, assuming USB is dead");

                // Fallback to bare USART
                if (SystemSettings.use_comm_uart) {
                    configure_interface(COMPORT_USART);
                }
                else if (SystemSettings.use_comm_nordic) {
                    configure_interface(COMPORT_NORDIC); // this fallbacks to LoRa if LoRa enabled
                }
                else if (SystemSettings.use_comm_lora) {
                    configure_interface(COMPORT_LORA);
                }
                else {
                    dbg("No alternate com interface configured, leaving USB enabled.");
                }
            } else {
                dbg("USB got address 0x%02x - OK", (int)uadr);
            }

            xfer_verify_done = true;
        }
    }
}

/** Claim resources that may be needed for alternate transfers */
void com_claim_resources_for_alt_transfers(void)
{
    if (SystemSettings.use_comm_uart) {
        do {
            if (E_SUCCESS != rsc_claim(&UNIT_SYSTEM, R_USART2)) {
                SystemSettings.use_comm_uart = false;
                break;
            }

            if (E_SUCCESS != rsc_claim(&UNIT_SYSTEM, R_PA2)) {
                SystemSettings.use_comm_uart = false;
                rsc_free(&UNIT_SYSTEM, R_USART2);
                break;
            }
            if (E_SUCCESS != rsc_claim(&UNIT_SYSTEM, R_PA3)) {
                SystemSettings.use_comm_uart = false;
                rsc_free(&UNIT_SYSTEM, R_USART2);
                rsc_free(&UNIT_SYSTEM, R_PA2);
                break;
            }
        } while (0);
    }
}

/** Release resources allocated for alternate transfers */
void com_release_resources_for_alt_transfers(void)
{
    if (SystemSettings.use_comm_uart) {
        rsc_free(&UNIT_SYSTEM, R_USART2);
        rsc_free(&UNIT_SYSTEM, R_PA2);
        rsc_free(&UNIT_SYSTEM, R_PA3);
    }
}

static uint32_t usart_rxi = 0;
static uint8_t usart_rx_buffer[MSG_QUE_SLOT_SIZE];
static uint32_t last_rx_time = 0;

/** Handler for the USART transport */
static void com_UsartIrqHandler(void *arg)
{
    (void)arg;
    if (LL_USART_IsActiveFlag_RXNE(USART2)) {
        vPortEnterCritical();
        {
            usart_rx_buffer[usart_rxi++] = LL_USART_ReceiveData8(USART2);
            if (usart_rxi == MSG_QUE_SLOT_SIZE) {
                rxQuePostMsg(usart_rx_buffer, MSG_QUE_SLOT_SIZE); // avoid it happening in the irq
                usart_rxi = 0;
            }
            last_rx_time = HAL_GetTick();
        }
        vPortExitCritical();
    }
}

/** this is called from the hal tick irq */
void com_iface_flush_buffer(void)
{
    if (usart_rxi > 0 && (HAL_GetTick()-last_rx_time)>=2) {
        vPortEnterCritical();
        {
            rxQuePostMsg(usart_rx_buffer, usart_rxi);
            usart_rxi = 0;
        }
        vPortExitCritical();
    }
}

static void configure_interface(enum ComportSelection iface)
{
    // Teardown
    if (gActiveComport == COMPORT_USB) {
        // simplest USB disabling (XXX needs porting)
        HAL_PCD_DeInit(&hpcd_USB_FS);
        __HAL_RCC_USB_CLK_DISABLE();
    }
    else if (gActiveComport == COMPORT_USART) {
        // this doesn't normally happen
        hw_deinit_pin_rsc(R_PA2);
        hw_deinit_pin_rsc(R_PA3);
        __HAL_RCC_USART2_CLK_DISABLE();
        irqd_detach(USART2, com_UsartIrqHandler);
    }
    gActiveComport = COMPORT_NONE;

    // Init
    if (iface == COMPORT_USB) {
        trap("illegal"); // this never happens
    }
    else if (iface == COMPORT_USART) {
        dbg("Setting up UART transfer");
        assert_param(E_SUCCESS == hw_configure_gpiorsc_af(R_PA2, LL_GPIO_AF_1));
        assert_param(E_SUCCESS == hw_configure_gpiorsc_af(R_PA3, LL_GPIO_AF_1));

        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_USART2_FORCE_RESET();
        __HAL_RCC_USART2_RELEASE_RESET();

        LL_USART_Disable(USART2);

        LL_USART_SetBaudRate(USART2, PLAT_APB1_HZ, LL_USART_OVERSAMPLING_16, SystemSettings.comm_uart_baud);
        dbg("baud = %d", (int)SystemSettings.comm_uart_baud);

        irqd_attach(USART2, com_UsartIrqHandler, NULL);
        LL_USART_EnableIT_RXNE(USART2);
        LL_USART_SetTransferDirection(USART2, LL_USART_DIRECTION_TX_RX);

        LL_USART_Enable(USART2);
    }
    else {
        if (iface == COMPORT_NORDIC) {
            // Try to configure nordic
            dbg("Setting up nRF transfer");

            // TODO set up and check nRF transport

            // On failure, try setting up LoRa
            dbg("nRF failed to init");
            if (SystemSettings.use_comm_lora) {
                iface = COMPORT_LORA;
            } else {
                iface = COMPORT_NONE; // fail
            }
        }

        if (iface == COMPORT_LORA) {
            // Try to configure nordic
            dbg("Setting up LoRa transfer");

            // TODO set up and check LoRa transport

            dbg("LoRa failed to init");
            iface = COMPORT_NONE; // fail
        }
    }

    if (iface == COMPORT_NONE) {
        dbg("NO COM PORT AVAILABLE!");
    }

    gActiveComport = iface;
}
