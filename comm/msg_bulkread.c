//
// Created by MightyPork on 2017/12/23.
//

#include "platform.h"

#include <TinyFrame.h>
#include "utils/malloc_safe.h"

#include "messages.h"
#include "utils/payload_parser.h"
#include "utils/payload_builder.h"

/**
 * TF listener for the bulk read transaction
 */
static TF_Result bulkread_lst(TinyFrame *tf, TF_Msg *msg)
{
    BulkRead *bulk = msg->userdata;

    // this is a final call before timeout, to clean up
    if (msg->data == NULL) {
        goto close;
    }

    assert_param(NULL != bulk);
    assert_param(NULL != bulk->_buffer);

    if (msg->type == MSG_BULK_ABORT) {
        goto close;
    }
    else if (msg->type == MSG_BULK_READ_POLL) {
        // find how much data the reader wants
        PayloadParser pp = pp_start(msg->data, msg->len, NULL);
        uint32_t chunk = pp_u32(&pp);

        chunk = MIN(chunk, bulk->len - bulk->offset);
        chunk = MIN(chunk, BULK_READ_BUF_LEN);

        // load data into the buffer
        bulk->read(bulk, chunk, bulk->_buffer);

        bool last = (bulk->offset + chunk >= bulk->len);

        TF_Msg resp;
        TF_ClearMsg(&resp);
        resp.frame_id = bulk->frame_id;
        resp.type = (last ? MSG_BULK_END : MSG_BULK_DATA); // the last chunk has the END type
        resp.data = bulk->_buffer;
        resp.len = (TF_LEN) chunk;
        TF_Respond(tf, &resp);

        if (last) goto close;

        // advance the position pointer
        bulk->offset += chunk;
    }

    return TF_RENEW;

close:
    if (bulk) {
        // Ask user to free the bulk and userdata
        bulk->read(bulk, 0, NULL);
        msg->userdata = NULL;
        free_ck(bulk->_buffer);
    }
    return TF_CLOSE;
}


void bulkread_start(TinyFrame *tf, BulkRead *bulk)
{
    assert_param(bulk);
    assert_param(bulk->len);
    assert_param(bulk->read);

    bulk->offset = 0;
    bulk->_buffer = malloc_ck(BULK_READ_BUF_LEN);

    {
        uint8_t buf[8];
        PayloadBuilder pb = pb_start(buf, 4, NULL);
        pb_u32(&pb, bulk->len);
        pb_u32(&pb, BULK_READ_BUF_LEN);

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
