//
// Created by MightyPork on 2017/12/22.
//

#include "platform.h"
#include "comm/messages.h"
#include "utils/hexdump.h"
#include "task_msg.h"
#include "sched_queue.h"

void TaskMessaging(const void * argument)
{
    dbg("> Message queue task started!");

    struct rx_que_item slot;
    while (1) {
        xQueueReceive(queRxDataHandle, &slot, osWaitForever);
        assert_param(slot.len>0 && slot.len<=64); // check the len is within bounds


        hexDump("MSG", slot.data, slot.len);

        TF_Accept(comm, slot.data, slot.len);
    }
}