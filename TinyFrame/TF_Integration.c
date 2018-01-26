//
// Created by MightyPork on 2017/11/21.
//
// TinyFrame integration
//

#include "platform.h"
#include "task_main.h"

#include "utils/hexdump.h"
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
        assert_param(osOK == osSemaphoreWait(semVcomTxReadyHandle, 1000));
        uint16_t chunksize = (uint16_t) MIN(total, CHUNK);
        assert_param(USBD_OK == CDC_Transmit_FS((uint8_t *) buff, chunksize));

        buff += chunksize;
        total -= chunksize;
    }
}

/** Claim the TX interface before composing and sending a frame */
bool TF_ClaimTx(TinyFrame *tf)
{
    (void) tf;
    assert_param(osThreadGetId() != tskMainHandle);
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
