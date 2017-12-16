//
// Created by MightyPork on 2017/11/21.
//

#include "platform.h"
#include "task_sched.h"

extern osMessageQId queSchedLPHandle;
extern osMessageQId queSchedHPHandle;

volatile uint32_t jobQueHighWaterMarkHP = 0;
volatile uint32_t jobQueHighWaterMarkLP = 0;

/**
 * Schedule a function for later execution in the jobs thread
 *
 * @param callback - the callback function
 * @param prio - priority, selects which job queue to use
 * @param data1 - first data object, NULL if not needed; usually context/handle
 * @param data2 - second data object, NULL if not needed; usually data/argument
 */
void scheduleJob(Job *job, enum task_sched_prio prio)
{
    QueueHandle_t que = (prio == TSK_SCHED_LOW ? queSchedLPHandle : queSchedHPHandle);

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
    if (prio == TSK_SCHED_LOW) {
        jobQueHighWaterMarkLP = MAX(jobQueHighWaterMarkLP, count);
    } else {
        jobQueHighWaterMarkHP = MAX(jobQueHighWaterMarkHP, count);
    }
#endif
}

/**
 * Low priority task queue handler
 *
 * @param argument
 */
void TaskSchedLP (const void * argument)
{
    dbg("> Low priority queue task started!");

    struct sched_que_item job;
    while (1) {
        xQueueReceive(queSchedLPHandle, &job, osWaitForever);

        assert_param(job.cb != NULL);
        job.cb(&job);
    }
}

/**
 * High priority task queue handler
 *
 * @param argument
 */
void TaskSchedHP (const void * argument)
{
    dbg("> High priority queue task started!");

    struct sched_que_item job;
    while (1) {
        xQueueReceive(queSchedHPHandle, &job, osWaitForever);

        assert_param(job.cb != NULL);
        job.cb(&job);
    }
}
