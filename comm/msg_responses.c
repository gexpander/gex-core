//
// Created by MightyPork on 2017/12/22.
//

#include "messages.h"
#include "msg_responses.h"

void com_respond_snprintf(TF_ID frame_id, TF_TYPE type, const char *format, ...)
{
#define ERR_STR_LEN 64

    char buf[ERR_STR_LEN];
    va_list args;
    va_start(args, format);
    uint32_t len = (uint32_t) fixup_vsnprintf(&buf[0], ERR_STR_LEN, format, args);
    va_end(args);

    com_respond_buf(frame_id, type, (const uint8_t *) buf, len);
}


void com_respond_buf(TF_ID frame_id, TF_TYPE type, const uint8_t *buf, uint32_t len)
{
    TF_Msg msg;
    TF_ClearMsg(&msg);
    {
        msg.type = type;
        msg.frame_id = frame_id;
        msg.data = buf;
        msg.len = (TF_LEN) len;
    }
    TF_Respond(comm, &msg);
}


void com_respond_ok(TF_ID frame_id)
{
    com_respond_buf(frame_id, MSG_SUCCESS, NULL, 0);
}


void com_send_buf(TF_TYPE type, const uint8_t *buf, uint32_t len)
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


void com_respond_str(TF_TYPE type, TF_ID frame_id, const char *str)
{
    com_respond_buf(frame_id, type, (const uint8_t *) str, (uint32_t) strlen(str));
}


// ---------------------------------------------------------------------------

void com_respond_err(TF_ID frame_id, const char *message)
{
    com_respond_str(MSG_ERROR, frame_id, message);
}


void com_respond_bad_cmd(TF_ID frame_id)
{
    com_respond_err(frame_id, "BAD COMMAND");
}


void com_respond_malformed_cmd(TF_ID frame_id)
{
    com_respond_err(frame_id, "MALFORMED PAYLOAD");
}

// ---------------------------------------------------------------------------

void com_respond_u8(TF_ID frame_id, uint8_t d)
{
    com_respond_buf(frame_id, MSG_SUCCESS, (const uint8_t *) &d, 1);
}


void com_respond_u16(TF_ID frame_id, uint16_t d)
{
    com_respond_buf(frame_id, MSG_SUCCESS, (const uint8_t *) &d, 2);
}


void com_respond_u32(TF_ID frame_id, uint32_t d)
{
    com_respond_buf(frame_id, MSG_SUCCESS, (const uint8_t *) &d, 4);
}