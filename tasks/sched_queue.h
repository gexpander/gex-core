//
// Created by MightyPork on 2017/11/21.
//

#ifndef GEX_SCHED_QUEUE_H
#define GEX_SCHED_QUEUE_H

#include "platform.h"
#include <TinyFrame.h>

typedef struct sched_que_item Job;
typedef void (*ScheduledJobCb) (Job *job);

struct sched_que_item {
    ScheduledJobCb cb;
    union {
        TF_ID frame_id;
        void *data1;
    };
    union {
        uint32_t d32;
        uint8_t *buf;
        const uint8_t *cbuf;
        const char *str;
        void *data2;
    };
    union {
        uint32_t len;
        void *data3;
    };
};

// This que is used to stash frames received from TinyFrame for later evaluation on the application thread
struct rx_que_item {
    uint32_t len;
    uint8_t data[MSG_QUE_SLOT_SIZE];
};

struct rx_sched_combined_que_item {
    bool is_job;
    union {
        struct rx_que_item msg;
        struct sched_que_item job;
    };
};

#endif //GEX_SCHED_QUEUE_H
