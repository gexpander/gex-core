//
// Created by MightyPork on 2017/12/23.
//

#include "platform.h"

#include <TinyFrame.h>

#include "messages.h"
#include "utils/payload_parser.h"
#include "utils/payload_builder.h"

static uint8_t bulkread_buffer[BULKREAD_MAX_CHUNK];

TF_Result bulkread_lst(TinyFrame *tf, TF_Msg *msg)
{
    // this is a final call before timeout, to clean up
    if (msg->data == NULL) {
        dbg("Bulk rx lst cleanup\r\n");
        goto close;
    }

    struct bulk_read *bulk = msg->userdata;
    assert_param(NULL != bulk);

    if (msg->type == MSG_BULK_ABORT) {
        goto close;
    } else if (msg->type == MSG_BULK_READ_POLL) {
        PayloadParser pp = pp_start(msg->data, msg->len, NULL);
        uint32_t chunk = pp_u32(&pp);

        // if past len, say we're done and close
        if (bulk->offset >= bulk->len) {
            TF_ClearMsg(msg);
            msg->frame_id = bulk->frame_id;
            msg->type = MSG_BULK_END;
            TF_Respond(tf, msg);
            goto close;
        }

        chunk = MIN(chunk, bulk->len - bulk->offset);
        chunk = MIN(chunk, BULKREAD_MAX_CHUNK);

        bulk->read(bulk->offset, chunk, bulkread_buffer);

        TF_ClearMsg(msg);
        msg->frame_id = bulk->frame_id;
        msg->type = MSG_BULK_DATA;
        msg->data = bulkread_buffer;
        msg->len = (TF_LEN) chunk;
        TF_Respond(tf, msg);

        // advance the position pointer
        bulk->offset += chunk;
    }

    msg->userdata = bulk; // We must put it back
    return TF_RENEW;

    close:
    if (msg->userdata) {
        free(msg->userdata);
        msg->userdata = NULL;
    }
    return TF_CLOSE;
}

void bulkread_start(TinyFrame *tf, struct bulk_read *bulk)
{
    assert_param(bulk);
    assert_param(bulk->len);
    assert_param(bulk->read);

    bulk->offset = 0;

    {
        uint8_t buf[8];
        PayloadBuilder pb = pb_start(buf, 4, NULL);
        pb_u32(&pb, bulk->len);
        pb_u32(&pb, BULKREAD_MAX_CHUNK);

        // We use userdata1 to hold a reference to the bulk transfer
        TF_Msg msg = {
                .type = MSG_BULK_READ_OFFER,
                .frame_id = bulk->frame_id,
                .is_response = true, // this ensures the frame_id is not re-generated
                .data = buf,
                .len = (TF_LEN) pb_length(&pb),
                .userdata = bulk,
        };

        TF_Query(tf, &msg, bulkread_lst, BULK_LST_TIMEOUT_MS);
    }
}
