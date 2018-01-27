//
// Created by MightyPork on 2018/01/27.
//

#include "platform.h"
#include "messages.h"
#include "event_reports.h"

static uint8_t evt_buf[10];

bool EventReport_Start(EventReport *report)
{
    assert_param(report->timestamp != 0);

    TF_Msg msg;
    TF_ClearMsg(&msg);
    msg.len = (TF_LEN) (report->length + 1 /*callsign*/ + 1 /*type*/ + 8 /*checksum*/);
    msg.type = MSG_UNIT_REPORT;
    if (!TF_Send_Multipart(comm, &msg)) {
        dbg("!! Err sending event");
        return false;
    }

    PayloadBuilder pb = pb_start(evt_buf, 10, NULL);
    pb_u8(&pb, report->unit->callsign);
    pb_u8(&pb, report->type);
    pb_u64(&pb, report->timestamp);
    assert_param(pb.ok);
    TF_Multipart_Payload(comm, evt_buf, 10);

    return true;
}

void EventReport_Data(const uint8_t *buff, uint16_t len)
{
    TF_Multipart_Payload(comm, buff, len);
}

void EventReport_End(void)
{
    TF_Multipart_Close(comm);
}

bool EventReport_Send(EventReport *report)
{
    assert_param(report->data != NULL);

    if (!EventReport_Start(report)) return false;
    EventReport_Data(report->data, report->length);
    EventReport_End();

    return true;
}
