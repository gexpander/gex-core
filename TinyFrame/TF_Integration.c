//
// Created by MightyPork on 2017/11/21.
//
// TinyFrame integration
//

#include "platform.h"
#include "task_main.h"
#include "comm/messages.h"
#include "comm/interfaces.h"
#include "comm/iface_uart.h"
#include "comm/iface_nordic.h"
#include "comm/iface_usb.h"
#include "framework/system_settings.h"

#include "USB/usbd_cdc_if.h"
#include "USB/usb_device.h"
#include "TinyFrame.h"

extern osSemaphoreId semVcomTxReadyHandle;
extern osMutexId mutTinyFrameTxHandle;

void TF_WriteImpl(TinyFrame *tf, const uint8_t *buff, uint32_t len)
{
    if (gActiveComport == COMPORT_USB) {
        iface_usb_transmit(buff, len);
    }
    else if (gActiveComport == COMPORT_USART) {
        iface_uart_transmit(buff, len);
    }
    else if (gActiveComport == COMPORT_NORDIC) {
        iface_nordic_transmit(buff, len);
    }
    else {
        // TODO other transports
        trap("not implemented.");
    }
}

/** Claim the TX interface before composing and sending a frame */
bool TF_ClaimTx(TinyFrame *tf)
{
    (void) tf;
    assert_param(!inIRQ());

    assert_param(pdTRUE == xSemaphoreTake(mutTinyFrameTxHandle, 5000)); // trips the wd

    // The last chunk from some previous frame may still be being transmitted,
    // wait for it to finish (the semaphore is given in the CDC tx done handler)
    if (pdTRUE != xSemaphoreTake(semVcomTxReadyHandle, 200)) {
        TF_Error("Tx stalled in Claim");

        // release the guarding mutex again
        assert_param(pdTRUE == xSemaphoreGive(mutTinyFrameTxHandle));
        return false;
    }

    return true;
}

/** Free the TX interface after composing and sending a frame */
void TF_ReleaseTx(TinyFrame *tf)
{
    (void) tf;
    assert_param(pdTRUE == xSemaphoreGive(mutTinyFrameTxHandle));

    // the last payload is sent asynchronously
}
