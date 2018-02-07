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

    const uint8_t len_overhead = 1 /*callsign*/ + 1 /*type*/ + 8 /*timestamp u64*/;

    TF_Msg msg = {
        .len = (TF_LEN) (report->length + len_overhead),
        .type = MSG_UNIT_REPORT,
    };

    if (!TF_Send_Multipart(comm, &msg)) {
        dbg("!! Err sending event");
        return false;
    }

    report->sent_msg_id = msg.frame_id;

    PayloadBuilder pb = pb_start(evt_buf, len_overhead, NULL);
    pb_u8(&pb, report->unit->callsign);
    pb_u8(&pb, report->type);
    pb_u64(&pb, report->timestamp);
    assert_param(pb.ok);
    TF_Multipart_Payload(comm, evt_buf, len_overhead);

    return true;
}

void EventReport_Data(const uint8_t *buff, uint16_t len)
{
    TF_Multipart_Payload(comm, buff, len);
}

void EventReport_PB(PayloadBuilder *pb)
{
    uint32_t len;
    uint8_t *buf = pb_close(pb, &len);
    TF_Multipart_Payload(comm, buf, len);
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
