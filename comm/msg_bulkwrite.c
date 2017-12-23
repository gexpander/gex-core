//
// Created by MightyPork on 2017/12/23.
//

#include "platform.h"

#include <TinyFrame.h>

#include "messages.h"
#include "utils/payload_parser.h"
#include "utils/payload_builder.h"

/**
 * TF listener for the bulk write transaction
 */
static TF_Result bulkwrite_lst(TinyFrame *tf, TF_Msg *msg)
{
    struct bulk_write *bulk = msg->userdata;

    // this is a final call before timeout, to clean up
    if (msg->data == NULL) {
        goto close;
    }

    assert_param(NULL != bulk);

    if (msg->type == MSG_BULK_ABORT) {
        goto close;
    }
    else if (msg->type == MSG_BULK_DATA || msg->type == MSG_BULK_END) {
        // if past len, say we're done and close
        if (bulk->offset >= bulk->len) {
            com_respond_err(bulk->frame_id, "WRITE OVERRUN");
            goto close;
        }

        bulk->write(bulk, msg->data, msg->len);

        // Return SUCCESS
        com_respond_ok(msg->frame_id);

        // advance the position pointer (can be used by the write callback)
        bulk->offset += msg->len;

        if (msg->type == MSG_BULK_END) {
            // this was the last chunk
            goto close;
        }
    }

    return TF_RENEW;

close:
    if (bulk) {
        // Ask user to free the bulk and userdata
        bulk->write(bulk, NULL, 0);
        msg->userdata = NULL;
    }
    return TF_CLOSE;
}

/** Start the bulk write flow */
void bulkwrite_start(TinyFrame *tf, struct bulk_write *bulk)
{
    assert_param(bulk);
    assert_param(bulk->len);
    assert_param(bulk->write);

    bulk->offset = 0;

    {
        uint8_t buf[8];
        PayloadBuilder pb = pb_start(buf, 8, NULL);
        pb_u32(&pb, bulk->len);
        pb_u32(&pb, TF_MAX_PAYLOAD_RX);

        // We use userdata1 to hold a reference to the bulk transfer
        TF_Msg msg = {
                .type = MSG_BULK_WRITE_OFFER,
                .frame_id = bulk->frame_id,
                .is_response = true, // this ensures the frame_id is not re-generated
                .data = buf,
                .len = (TF_LEN) pb_length(&pb),
                .userdata = bulk,
        };

        TF_Query(tf, &msg, bulkwrite_lst, BULK_LST_TIMEOUT_MS);
    }
}
