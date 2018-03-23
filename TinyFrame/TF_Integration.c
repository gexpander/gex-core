//
// Created by MightyPork on 2017/11/21.
//
// TinyFrame integration
//

#include "platform.h"
#include "task_main.h"
#include "comm/messages.h"
#include "comm/interfaces.h"
#include "framework/system_settings.h"

#include "USB/usbd_cdc_if.h"
#include "USB/usb_device.h"
#include "TinyFrame.h"

extern osSemaphoreId semVcomTxReadyHandle;
extern osMutexId mutTinyFrameTxHandle;

/**
 * USB transmit implementation
 *
 * @param tf - TF
 * @param buff - buffer to send (can be longer than the buffers)
 * @param len - buffer size
 */
static inline void _USB_WriteImpl(TinyFrame *tf, const uint8_t *buff, uint32_t len)
{
#if 1
    const uint32_t real_size = len;

    // Padding to a multiple of 64 bytes - this is supposed to maximize the bulk transfer speed
    if ((len&0x3F) && !SystemSettings.visible_vcom) { // this corrupts VCOM on Linux for some reason
        uint32_t pad = (64 - (len&0x3F));
        memset((void *) (buff + len), 0, pad);
        len += pad; // padding to a multiple of 64 (size of the endpoint)
    }

    // We bypass the USBD driver library's overhead by using the HAL function directly
    assert_param(HAL_OK == HAL_PCD_EP_Transmit(hUsbDeviceFS.pData, CDC_IN_EP, (uint8_t *) buff, len));

    // The buffer is the TF transmit buffer, we can't leave it to work asynchronously because
    // the next call could modify it before it's been transmitted (in the case of a chunked / multi-part frame)

    // If this is not the last chunk (assuming all but the last use full 512 bytes of the TF buffer), wait now for completion
    if (real_size == TF_SENDBUF_LEN) {
        // TODO this seems wrong - investigate
        if (pdTRUE != xSemaphoreTake(semVcomTxReadyHandle, 100)) {
            TF_Error("Tx stalled in WriteImpl");
            return;
        }
    }
#else
    (void) tf;
#define CHUNK 64 // size of the USB packet
    int32_t total = (int32_t) len;
    while (total > 0) {
        const int32_t mxStatus = osSemaphoreWait(semVcomTxReadyHandle, 100);
        if (mxStatus != osOK) {
            TF_Error("Tx stalled");
            return;
        }

        const uint16_t chunksize = (uint16_t) MIN(total, CHUNK);

        // this is an attempt to speed it up a little by removing a couple levels of indirection
        assert_param(HAL_OK == HAL_PCD_EP_Transmit(hUsbDeviceFS.pData, CDC_IN_EP, (uint8_t *) buff, chunksize));

//        USBD_LL_Transmit(&hUsbDeviceFS, CDC_IN_EP, (uint8_t *) buff, chunksize);
//        assert_param(USBD_OK == CDC_Transmit_FS((uint8_t *) buff, chunksize));

        buff += chunksize;
        total -= chunksize;
    }
#endif
}

void TF_WriteImpl(TinyFrame *tf, const uint8_t *buff, uint32_t len)
{
    if (gActiveComport == COMPORT_USB) {
        _USB_WriteImpl(tf, buff, len);
    }
    else if (gActiveComport == COMPORT_USART) {
        // TODO rewrite this to use DMA, then wait for the DMA
        for(uint32_t i=0;i<len;i++) {
            while(!LL_USART_IsActiveFlag_TXE(USART2));
            LL_USART_TransmitData8(USART2, buff[i]);
        }
        xSemaphoreGive(semVcomTxReadyHandle); // act as if we just finished it and this is perhaps the DMA irq
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
    if (pdTRUE != xSemaphoreTake(semVcomTxReadyHandle, 100)) {
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
