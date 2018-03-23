//
// Created by MightyPork on 2017/11/09.
//

#include "platform.h"
#include "platform/lock_jumper.h"
#include "platform/watchdog.h"
#include "status_led.h"
#include "utils/stacksmon.h"
#include "vfs/vfs_manager.h"
#include "usbd_cdc.h"
#include "usb_device.h"
#include "usbd_msc.h"
#include "task_main.h"
#include "comm/interfaces.h"

/* TaskUsbEvent function */
void TaskMain(void const * argument)
{
    dbg("> Main task started!");

#ifndef DISABLE_MSC
    vfs_if_usbd_msc_init();
    vfs_mngr_init(1);
#endif

    Indicator_Effect(STATUS_WELCOME);

    const uint32_t bootTime = HAL_GetTick();
    uint32_t startTime = bootTime;
    uint32_t cnt = 1;
    bool waiting_for_usb = true;
    while(1) {
        uint32_t msg;
        xTaskNotifyWait(0, UINT32_MAX, &msg, 100); // time out if nothing happened

        // periodic updates to the VFS driver
        uint32_t now = HAL_GetTick();
        uint32_t elapsed = now - startTime;
        if (elapsed >= 100) {
            // interval 100ms or more - slow periodic

#ifndef DISABLE_MSC
            vfs_mngr_periodic(elapsed);
#endif

            LockJumper_Check();
            startTime = now;

            cnt++;
            Indicator_Heartbeat();

            wd_restart();

            // If USB has no signal, set up alternate communication interface
            com_switch_transfer_if_needed();
        }

        // if no message and it just timed out, go wait some more...
        if (msg == 0) {
            // Low priority periodic tasks... (TODO move to a timer?)

            // Periodically check stacks for overrun
            stackmon_check_canaries();
            // Periodically dump all stacks - for checking levels before critical (to reduce size if not needed)
            if ((cnt%150)==0) stackmon_dump();
            continue;
        }

        if (gActiveComport == COMPORT_USB) {
            // Endpoint 0 - control messages for the different classes
            if (msg & USBEVT_FLAG_EP0_RX_RDY) {
                USBD_CDC_EP0_RxReady(&hUsbDeviceFS);
            }

            if (msg & USBEVT_FLAG_EP0_TX_SENT) {
                //
            }

#ifndef DISABLE_MSC
            // MSC - read/write etc
            if (msg & (USBEVT_FLAG_EPx_IN(MSC_EPIN_ADDR))) {
                USBD_MSC_DataIn(&hUsbDeviceFS, MSC_EPIN_ADDR);
            }

            if (msg & (USBEVT_FLAG_EPx_OUT(MSC_EPOUT_ADDR))) {
                USBD_MSC_DataOut(&hUsbDeviceFS, MSC_EPOUT_ADDR);
            }
#endif

            // CDC - config packets and data in/out
//          if (msg & (USBEVT_FLAG_EPx_IN(CDC_IN_EP))) {
//              USBD_CDC_DataIn(&hUsbDeviceFS, CDC_IN_EP);
//          }

            if (msg & (USBEVT_FLAG_EPx_IN(CDC_CMD_EP))) {
                USBD_CDC_DataIn(&hUsbDeviceFS, CDC_CMD_EP);
            }

            if (msg & (USBEVT_FLAG_EPx_OUT(CDC_OUT_EP))) {
                USBD_CDC_DataOut(&hUsbDeviceFS, CDC_OUT_EP);
            }
        }
    }
}
