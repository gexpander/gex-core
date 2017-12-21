//
// Created by MightyPork on 2017/11/21.
//

#include "platform.h"
#include "TinyFrame.h"
#include "framework/unit_registry.h"
#include "comm/messages.h"
#include "task_sched.h"

TinyFrame tf_;
TinyFrame *comm = &tf_;

// ---------------------------------------------------------------------------

// Pre-declaring local functions
static TF_Result lst_ping(TinyFrame *tf, TF_Msg *msg);
static TF_Result lst_unit(TinyFrame *tf, TF_Msg *msg);
static TF_Result lst_list_units(TinyFrame *tf, TF_Msg *msg);
static TF_Result lst_default(TinyFrame *tf, TF_Msg *msg);

void comm_init(void)
{
    TF_InitStatic(comm, TF_SLAVE);
    TF_AddTypeListener(comm, MSG_PING, lst_ping);
    TF_AddTypeListener(comm, MSG_UNIT_REQUEST, lst_unit);
    TF_AddTypeListener(comm, MSG_LIST_UNITS, lst_list_units);

    // fall-through
    TF_AddGenericListener(comm, lst_default);
}

// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------

static void job_ping_reply(Job *job)
{
    tf_respond_snprintf(MSG_SUCCESS, job->frame_id, "%s/%s", GEX_VERSION, GEX_PLATFORM);
}

static TF_Result lst_ping(TinyFrame *tf, TF_Msg *msg)
{
    Job job = {
        .cb = job_ping_reply,
        .frame_id = msg->frame_id
    };
    scheduleJob(&job, TSK_SCHED_LOW);

    return TF_STAY;
}

// ----------------------------------------------------------------------------

static void job_unhandled_resp(Job *job)
{
    tf_respond_snprintf(MSG_ERROR, job->frame_id, "UNKNOWN MSG %"PRIu32, job->d32);
}

static TF_Result lst_default(TinyFrame *tf, TF_Msg *msg)
{
    dbg("!! Unhandled msg type %d, frame_id %d", (int)msg->type, (int)msg->frame_id);
    Job job = {
        .cb = job_unhandled_resp,
        .frame_id = msg->frame_id,
        .d32 = msg->type
    };
    scheduleJob(&job, TSK_SCHED_LOW);

    return TF_STAY;
}

// ----------------------------------------------------------------------------

static TF_Result lst_unit(TinyFrame *tf, TF_Msg *msg)
{
    ureg_deliver_unit_request(msg);
    return TF_STAY;
}

// ----------------------------------------------------------------------------

static void job_list_units(Job *job)
{
    ureg_report_active_units(job->frame_id);
}

static TF_Result lst_list_units(TinyFrame *tf, TF_Msg *msg)
{
    Job job = {
        .cb = job_list_units,
        .frame_id = msg->frame_id,
    };
    scheduleJob(&job, TSK_SCHED_LOW);

    return TF_STAY;
}
