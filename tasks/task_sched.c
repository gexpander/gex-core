//
// Created by MightyPork on 2017/11/21.
//

#include "platform.h"
#include "task_sched.h"

extern osMessageQId queSchedHandle;

volatile uint32_t jobQueHighWaterMark = 0;

/**
 * Schedule a function for later execution in the jobs thread
 *
 * @param callback - the callback function
 */
void scheduleJob(Job *job)
{
    QueueHandle_t que = queSchedHandle;

    assert_param(que != NULL);
    assert_param(job->cb != NULL);

    uint32_t count;
    if (inIRQ()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        assert_param(pdPASS == xQueueSendFromISR(que, job, &xHigherPriorityTaskWoken));
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

#if USE_STACK_MONITOR
        count = (uint32_t) uxQueueMessagesWaitingFromISR(que);
#endif
    } else {
        assert_param(pdPASS == xQueueSend(que, job, 100));

#if USE_STACK_MONITOR
        count = (uint32_t) uxQueueMessagesWaiting(que);
#endif
    }

#if USE_STACK_MONITOR
    jobQueHighWaterMark = MAX(jobQueHighWaterMark, count);
#endif
}

/**
 * job queue handler (for use in interrupts to do longer stuff on a thread)
 *
 * @param argument
 */
void TaskJobQueue(const void *argument)
{
    dbg("> High priority queue task started!");

    struct sched_que_item job;
    while (1) {
        xQueueReceive(queSchedHandle, &job, osWaitForever);

        assert_param(job.cb != NULL);
        job.cb(&job);
    }
}
