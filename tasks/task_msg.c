//
// Created by MightyPork on 2017/12/22.
//

#include "platform.h"
#include "comm/messages.h"
#include "task_msg.h"

volatile uint32_t msgQueHighWaterMark = 0;

/**
 * Process data received from TinyFrame.
 * The queue holds received messages or parts of messages,
 * copied there by the USB thread.
 *
 * Since this is a separate thread that handles TF input and runs the listeners,
 * TF functions (send, respond) can be called immediately without the need for an
 * intermediate queued job.
 */
void TaskMessaging(const void * argument)
{
    dbg("> Message queue task started!");

    struct rx_que_item slot;
    while (1) {
        xQueueReceive(queRxDataHandle, &slot, osWaitForever);
        assert_param(slot.len>0 && slot.len<=64); // check the len is within bounds

        // We need thr scratch buffer for many unit command handlers
        TF_Accept(comm, slot.data, slot.len);

#if USE_STACK_MONITOR
        uint32_t count;
        count = (uint32_t) uxQueueMessagesWaiting(queRxDataHandle); // this seems to return N+1, hence we don't add the +1 for the one just removed.
        msgQueHighWaterMark = MAX(msgQueHighWaterMark, count);
#endif
    }
}
