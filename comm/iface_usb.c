//
// Created by MightyPork on 2018/04/06.
//

#include "iface_usb.h"
#include "TinyFrame.h"
#include "system_settings.h"
#include "USB/usb_device.h"

extern osSemaphoreId semVcomTxReadyHandle;
extern osMutexId mutTinyFrameTxHandle;

bool iface_usb_ready(void)
{
    return 0 != (USB->DADDR & USB_DADDR_ADD);
}

/**
 * USB transmit implementation
 *
 * @param buff - buffer to send (can be longer than the buffers)
 * @param len - buffer size
 */
void iface_usb_transmit(const uint8_t *buff, uint32_t len)
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
