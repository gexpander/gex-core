//
// Created by MightyPork on 2017/11/21.
//
// Job and message scheduling queue structs and typedefs
//

#ifndef GEX_SCHED_QUEUE_H
#define GEX_SCHED_QUEUE_H

#include "platform.h"
#include <TinyFrame.h>

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
    /** Data word 1 */
    union {
        TF_ID frame_id; // typically used to pass frame id to the callback
        void *data1; // arbitrary pointer or int
        uint32_t d32_1; // passing a number
    };
    /** Data word 2 */
    union {
        uint32_t d32; // passing a number
        uint32_t d32_2; // passing a number
        uint8_t *buf; // uchar buffer
        const uint8_t *cbuf; // const uchar buffer
        const char *str; // string
        void *data2; // arbitrary pointer
    };
    /** Data word 3 */
    union {
        uint32_t len; // typically length of the buffer
        void *data3; // arbitrary pointer
        uint32_t d32_3; // passing a number
    };
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
