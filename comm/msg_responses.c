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


void tf_respond_ok(TF_ID frame_id)
{
    tf_respond_buf(MSG_SUCCESS, frame_id, NULL, 0);
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


void tf_respond_str(TF_TYPE type, TF_ID frame_id, const char *str)
{
    tf_respond_buf(type, frame_id, (const uint8_t *) str, (uint32_t) strlen(str));
}


// ---------------------------------------------------------------------------

void tf_respond_err(TF_ID frame_id, const char *message)
{
    tf_respond_str(MSG_ERROR, frame_id, message);
}


void tf_respond_bad_cmd(TF_ID frame_id)
{
    tf_respond_err(frame_id, "BAD COMMAND");
}


void tf_respond_malformed_cmd(TF_ID frame_id)
{
    tf_respond_err(frame_id, "MALFORMED PAYLOAD");
}

// ---------------------------------------------------------------------------

void tf_respond_u8(TF_ID frame_id, uint8_t d)
{
    tf_respond_buf(MSG_SUCCESS, frame_id, (const uint8_t *) &d, 1);
}


void tf_respond_u16(TF_ID frame_id, uint16_t d)
{
    tf_respond_buf(MSG_SUCCESS, frame_id, (const uint8_t *) &d, 2);
}


void tf_respond_u32(TF_ID frame_id, uint32_t d)
{
    tf_respond_buf(MSG_SUCCESS, frame_id, (const uint8_t *) &d, 4);
}
