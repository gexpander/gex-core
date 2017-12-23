//
// Created by MightyPork on 2017/11/21.
//

#ifndef GEX_TASK_SCHED_H
#define GEX_TASK_SCHED_H

#include "platform.h"
#include "sched_queue.h"

#if USE_STACK_MONITOR
extern volatile uint32_t jobQueHighWaterMark;
#endif

extern osThreadId tskJobRunnerHandle;
void TaskJobQueue(const void *argument);

void scheduleJob(Job *job);

#endif //GEX_TASK_SCHED_H
