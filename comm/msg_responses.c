//
// Created by MightyPork on 2017/12/22.
//

#include "messages.h"
#include "msg_responses.h"

void tf_respond_snprintf(TF_TYPE type, TF_ID id, const char *format, ...)
{
#define ERR_STR_LEN 64
    char buf[ERR_STR_LEN];
    va_list args;
    va_start(args, format);
    uint32_t len = (uint32_t) fixup_vsnprintf(&buf[0], ERR_STR_LEN, format, args);
    va_end(args);

    tf_respond_buf(type, id, (const uint8_t *) buf, len);
}


void tf_respond_buf(TF_TYPE type, TF_ID id, const uint8_t *buf, uint32_t len)
{
    TF_Msg msg;
    TF_ClearMsg(&msg);
    {
        msg.type = type;
        msg.frame_id = id;
        msg.data = buf;
        msg.len = (TF_LEN) len;
    }
    TF_Respond(comm, &msg);
}


void tf_send_buf(TF_TYPE type, const uint8_t *buf, uint32_t len)
{
    TF_Msg msg;
    TF_ClearMsg(&msg);
    {
        msg.type = MSG_UNIT_REPORT;
        msg.data = buf;
        msg.len = (TF_LEN) len;
        msg.type = type;
    }
    TF_Send(comm, &msg); // no listener
}


// ---------------------------------------------------------------------------

static void job_respond_err(Job *job)
{
    tf_respond_str(MSG_ERROR, job->frame_id, job->str);
}


void sched_respond_err(TF_ID frame_id, const char *message)
{
    dbg("ERR: %s", message);
    Job job = {
            .cb = job_respond_err,
            .frame_id = frame_id,
            .str = message
    };
    scheduleJob(&job, TSK_SCHED_LOW);
}


void sched_respond_bad_cmd(TF_ID frame_id)
{
    sched_respond_err(frame_id, "BAD COMMAND");
}


void sched_respond_malformed_cmd(TF_ID frame_id)
{
    sched_respond_err(frame_id, "MALFORMED PAYLOAD");
}

// ---------------------------------------------------------------------------

static void job_respond_suc(Job *job)
{
    tf_respond_ok(job->frame_id);
}


void sched_respond_suc(TF_ID frame_id)
{
    Job job = {
            .cb = job_respond_suc,
            .frame_id = frame_id
    };
    scheduleJob(&job, TSK_SCHED_LOW);
}

// ---------------------------------------------------------------------------

static void job_respond_uX(Job *job)
{
    tf_respond_buf(MSG_SUCCESS, job->frame_id, (const uint8_t *) &job->d32, job->len);
}


void sched_respond_u8(TF_ID frame_id, uint8_t d)
{
    Job job = {
            .cb = job_respond_uX,
            .frame_id = frame_id,
            .d32 = d,
            .len = 1
    };
    scheduleJob(&job, TSK_SCHED_HIGH);
}


void sched_respond_u16(TF_ID frame_id, uint16_t d)
{
    Job job = {
            .cb = job_respond_uX,
            .frame_id = frame_id,
            .d32 = d,
            .len = 2
    };
    scheduleJob(&job, TSK_SCHED_HIGH);
}


void sched_respond_u32(TF_ID frame_id, uint32_t d)
{
    Job job = {
            .cb = job_respond_uX,
            .frame_id = frame_id,
            .d32 = d,
            .len = 4
    };
    scheduleJob(&job, TSK_SCHED_HIGH);
}
