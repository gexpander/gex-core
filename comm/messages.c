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

static TF_Result lst_ping(TinyFrame *tf, TF_Msg *msg)
{
    tf_respond_snprintf(MSG_SUCCESS, msg->frame_id, "%s/%s", GEX_VERSION, GEX_PLATFORM);
    return TF_STAY;
}

// ----------------------------------------------------------------------------

static TF_Result lst_default(TinyFrame *tf, TF_Msg *msg)
{
    dbg("!! Unhandled msg type %02"PRIx8", frame_id 0x%04"PRIx16, msg->type, msg->frame_id);

    tf_respond_snprintf(MSG_ERROR, msg->frame_id, "UNKNOWN MSG %"PRIu8, msg->type);
    return TF_STAY;
}

// ----------------------------------------------------------------------------

static TF_Result lst_unit(TinyFrame *tf, TF_Msg *msg)
{
    ureg_deliver_unit_request(msg);
    return TF_STAY;
}

// ----------------------------------------------------------------------------

static TF_Result lst_list_units(TinyFrame *tf, TF_Msg *msg)
{
    ureg_report_active_units(msg->frame_id);
    return TF_STAY;
}
