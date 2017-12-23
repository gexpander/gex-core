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
    dbg("!! Unhandled msg type %02"PRIx8", frame_id 0x%04"PRIx16, msg->type, msg->frame_id);
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
