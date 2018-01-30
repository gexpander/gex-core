//
// Created by MightyPork on 2017/12/22.
//
// Routines for sending TinyFrame responses.
//

#ifndef GEX_F072_MSG_RESPONSES_H
#define GEX_F072_MSG_RESPONSES_H

#ifndef GEX_MESSAGES_H
#error "Include messages.h instead!"
#endif

#include "payload_builder.h"

/**
 * Respond to a TF message using printf-like formatting.
 *
 * @param type - response type byte
 * @param frame_id - ID of the original msg
 * @param format - printf format
 * @param ... - replacements
 */
void  __attribute__((format(printf,3,4)))
com_respond_snprintf(TF_ID frame_id, TF_TYPE type, const char *format, ...);

/**
 * Respond to a TF message with a buffer of fixed length and custom type.
 *
 * @param type - response type byte
 * @param frame_id - ID of the original msg
 * @param buf - byte buffer
 * @param len - buffer size
 */
void com_respond_buf(TF_ID frame_id, TF_TYPE type, const uint8_t *buf, uint32_t len);

/**
 * Respond using a payload builder's content
 *
 * @param type - response type byte
 * @param frame_id - ID of the original msg
 * @param pb - builder
 */
void com_respond_pb(TF_ID frame_id, TF_TYPE type, PayloadBuilder *pb);

/**
 * Respond to a TF message with empty body and MSG_SUCCESS type.
 *
 * @param frame_id  - ID of the original msg
 */
void com_respond_ok(TF_ID frame_id);

/**
 * Respond with a error constant (converted to string)
 *
 * @param frame_id - ID of the original msg
 * @param error - error to report
 */
void com_respond_error(TF_ID frame_id, error_t error);

/**
 * Same like tf_respond_buf(), but used for sending spontaneous reports.
 *
 * @param type - response type byte
 * @param buf - byte buffer
 * @param len - buffer size
 */
void com_send_buf(TF_TYPE type, const uint8_t *buf, uint32_t len);

/**
 * Send a payload builder's content
 *
 * @param type - response type byte
 * @param pb - builder
 */
void com_send_pb(TF_TYPE type, PayloadBuilder *pb);

/**
 * Same like tf_respond_buf(), but the buffer length is measured with strlen.
 * Used to sending ASCII string responses.
 *
 * @param type - response type byte
 * @param frame_id - ID of the original msg
 * @param str - character buffer, zero terminated
 */
void com_respond_str(TF_TYPE type, TF_ID frame_id, const char *str);

/**
 * Schedule sending a one-byte response with MSG_SUCCESS type.
 *
 * @param frame_id - ID of the original msg
 * @param d - data
 */
void com_respond_u8(TF_ID frame_id, uint8_t d);

/**
 * Schedule sending a two-byte response with MSG_SUCCESS type.
 *
 * @param frame_id - ID of the original msg
 * @param d - data
 */
void com_respond_u16(TF_ID frame_id, uint16_t d);

/**
 * Schedule sending a 4-byte response with MSG_SUCCESS type.
 *
 * @param frame_id - ID of the original msg
 * @param d - data
 */
void com_respond_u32(TF_ID frame_id, uint32_t d);

#endif //GEX_F072_MSG_RESPONSES_H
