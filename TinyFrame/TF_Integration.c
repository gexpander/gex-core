//
// Created by MightyPork on 2017/11/21.
//
// TinyFrame integration
//

#include <USB/usb_device.h>
#include "platform.h"
#include "task_main.h"

#include "USB/usbd_cdc_if.h"
#include "TinyFrame.h"

extern osSemaphoreId semVcomTxReadyHandle;
extern osMutexId mutTinyFrameTxHandle;

void TF_WriteImpl(TinyFrame *tf, const uint8_t *buff, uint32_t len)
{
    (void) tf;
#define CHUNK 64 // same as TF_SENDBUF_LEN, so we should always have only one run of the loop
    int32_t total = (int32_t) len;
    while (total > 0) {
        const int32_t mxStatus = osSemaphoreWait(semVcomTxReadyHandle, 100);
        if (mxStatus != osOK) {
            TF_Error("Tx stalled");
            return;
        }

        const uint16_t chunksize = (uint16_t) MIN(total, CHUNK);

        // this is an attempt to speed it up a little by removing a couple levels of indirection
//        assert_param(HAL_OK == HAL_PCD_EP_Transmit(hUsbDeviceFS.pData, CDC_IN_EP, (uint8_t *) buff, chunksize));

//        USBD_LL_Transmit(&hUsbDeviceFS, CDC_IN_EP, (uint8_t *) buff, chunksize);
        assert_param(USBD_OK == CDC_Transmit_FS((uint8_t *) buff, chunksize));

        buff += chunksize;
        total -= chunksize;
    }
}

/** Claim the TX interface before composing and sending a frame */
bool TF_ClaimTx(TinyFrame *tf)
{
    (void) tf;
//    assert_param(osThreadGetId() != tskMainHandle);
    assert_param(!inIRQ());

    assert_param(osOK == osMutexWait(mutTinyFrameTxHandle, 5000));
    return true;
}

/** Free the TX interface after composing and sending a frame */
void TF_ReleaseTx(TinyFrame *tf)
{
    (void) tf;
    assert_param(osOK == osMutexRelease(mutTinyFrameTxHandle));
}
