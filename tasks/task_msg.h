//
// Created by MightyPork on 2017/12/22.
//
// Message and job combined queue, used for async job execution (dispatched form ISR)
// and TinyFrame message handling
//

#ifndef GEX_CORE_TASK_MSG_H
#define GEX_CORE_TASK_MSG_H

#include "platform.h"
#include "sched_queue.h"

/** Message queue handle */
extern osMessageQId queMsgJobHandle;

/** Message/job thread handle */
extern osThreadId tskMsgJobHandle;

/** Message/job thread entry point */
void TaskMsgJob(const void *argument);

/**
 * Add a job to the queue
 *
 * @param job
 */
bool scheduleJob(Job *job);

/**
 * Add a message to the queue. This always takes the entire slot (64 bytes) or multiple
 *
 * @param buf - byte buffer
 * @param len - number of bytes; can exceed 64 - then it will be split to multiple slots
 */
void rxQuePostMsg(uint8_t *buf, uint32_t len);

#if USE_STACK_MONITOR
extern volatile uint32_t msgQueHighWaterMark;
#endif

#endif //GEX_CORE_TASK_MSG_H
