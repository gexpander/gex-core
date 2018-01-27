//
// Created by MightyPork on 2017/11/21.
//
// Job and message scheduling queue structs and typedefs
//

#ifndef GEX_SCHED_QUEUE_H
#define GEX_SCHED_QUEUE_H

#include "platform.h"
#include <TinyFrame.h>
#include "framework/unit.h"

/**
 * Queued job typedef
 */
typedef struct sched_que_item Job;

/**
 * Queued job callback
 */
typedef void (*ScheduledJobCb) (Job *job);

/**
 * Queued job data struct
 */
struct sched_que_item {
    /** The callback */
    ScheduledJobCb cb;

    // Fields for data
    Unit *unit;
    uint64_t timestamp;
    uint32_t data1;
    uint32_t data2;
    uint32_t data3;
};

/**
 * Queued data chunk structure - used for buffering received messages for TinyFrame
 */
struct rx_que_item {
    uint32_t len;
    uint8_t data[MSG_QUE_SLOT_SIZE];
};

/**
 * Combined struct - added when we merged the two separate queues
 */
struct rx_sched_combined_que_item {
    bool is_job; // tag indicating whether the job or msg field is used
    union {
        struct rx_que_item msg;
        struct sched_que_item job;
    };
};

#endif //GEX_SCHED_QUEUE_H
