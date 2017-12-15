//
// Created by MightyPork on 2017/11/21.
//

#ifndef GEX_TASK_SCHED_H
#define GEX_TASK_SCHED_H

#include "platform.h"
#include "sched_queue.h"

enum task_sched_prio {
    TSK_SCHED_LOW = 0,
    TSK_SCHED_HIGH = 1,
};

#if USE_STACK_MONITOR
extern volatile uint32_t jobQueHighWaterMarkHP;
extern volatile uint32_t jobQueHighWaterMarkLP;
#endif

extern osThreadId tskSchedLPHandle;
void TaskSchedLP (const void * argument);

extern osThreadId tskSchedHPHandle;
void TaskSchedHP (const void * argument);

void scheduleJob(Job *job, enum task_sched_prio prio);

#endif //GEX_TASK_SCHED_H
