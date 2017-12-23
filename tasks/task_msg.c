//
// Created by MightyPork on 2017/12/22.
//

#include "platform.h"
#include "comm/messages.h"
#include "utils/hexdump.h"
#include "task_msg.h"
#include "sched_queue.h"

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

        TF_Accept(comm, slot.data, slot.len);
    }
}