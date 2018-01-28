//
// Created by MightyPork on 2017/12/22.
//

#include "platform.h"
#include "comm/messages.h"
#include "task_msg.h"

volatile uint32_t msgQueHighWaterMark = 0;

static void que_safe_post(struct rx_sched_combined_que_item *slot)
{
    uint32_t count = 0;
    assert_param(slot != NULL);

    if (inIRQ()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        BaseType_t status = xQueueSendFromISR(queMsgJobHandle, slot, &xHigherPriorityTaskWoken);
        if (pdPASS != status) {
            dbg("! Que post from ISR failed");
            return;
        }

        #if USE_STACK_MONITOR
            count = (uint32_t) uxQueueMessagesWaitingFromISR(queMsgJobHandle);
        #endif

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        BaseType_t status = xQueueSend(queMsgJobHandle, slot, MSG_QUE_POST_TIMEOUT);
        if (pdPASS != status) {
            dbg("! Que post failed");
            return;
        }

        #if USE_STACK_MONITOR
            count = (uint32_t) uxQueueMessagesWaiting(queMsgJobHandle);
        #endif
    }

    #if USE_STACK_MONITOR
        msgQueHighWaterMark = MAX(msgQueHighWaterMark, count);
    #endif
}

/**
 * Schedule a function for later execution in the jobs thread
 *
 * @param callback - the callback function
 */
void scheduleJob(Job *job)
{
    assert_param(job->cb != NULL);

    struct rx_sched_combined_que_item slot = {
        .is_job = true,
        .job = *job, // copy content of the struct
    };

    que_safe_post(&slot);
}

/**
 * Process data received from TinyFrame.
 * The queue holds received messages or parts of messages,
 * copied there by the USB thread.
 *
 * Since this is a separate thread that handles TF input and runs the listeners,
 * TF functions (send, respond) can be called immediately without the need for an
 * intermediate queued job.
 */
void TaskMsgJob(const void *argument)
{
    dbg("> Job+Msg queue task started!");

    struct rx_sched_combined_que_item slot;
    while (1) {
        xQueueReceive(queMsgJobHandle, &slot, osWaitForever);

        if (slot.is_job) {
            assert_param(slot.job.cb != NULL);
            slot.job.cb(&slot.job);
        }
        else {
            assert_param(slot.msg.len > 0 && slot.msg.len <= MSG_QUE_SLOT_SIZE); // check the len is within bounds

            #if CDC_LOOPBACK_TEST
                TF_WriteImpl(comm, slot.msg.data, slot.msg.len);
            #else
                TF_Accept(comm, slot.msg.data, slot.msg.len);
            #endif
        }

        #if USE_STACK_MONITOR
            uint32_t count;
            count = (uint32_t) uxQueueMessagesWaiting(queMsgJobHandle)+1;
            msgQueHighWaterMark = MAX(msgQueHighWaterMark, count);
        #endif
    }
}

void rxQuePostMsg(uint8_t *buf, uint32_t len)
{
    assert_param(buf != NULL);
    assert_param(len != 0);

    static struct rx_sched_combined_que_item slot;

    do {
        // Post the data chunk on the RX queue to be handled asynchronously.
        slot.is_job = false;
        slot.msg.len = len > MSG_QUE_SLOT_SIZE ? MSG_QUE_SLOT_SIZE : len;
        memcpy(slot.msg.data, buf, slot.msg.len);

        que_safe_post(&slot);

        len -= slot.msg.len;
        buf += slot.msg.len;
    } while (len > 0);
}
