//
// Created by MightyPork on 2017/12/22.
//

#ifndef GEX_F072_TASK_MSG_H
#define GEX_F072_TASK_MSG_H

#include "platform.h"
#include "sched_queue.h"

extern osMessageQId queMsgJobHandle;
extern osThreadId tskMsgJobHandle;
void TaskMsgJob(const void *argument);
void scheduleJob(Job *job);

#if USE_STACK_MONITOR
extern volatile uint32_t msgQueHighWaterMark;
#endif

void rxQuePostMsg(uint8_t *buf, uint32_t len);

#endif //GEX_F072_TASK_MSG_H
