//
// Created by MightyPork on 2017/11/21.
//
// This is the only file to include from the comm/ folder directly.
// Provides TinyFrame message types, utilities for forming payloads,
// bulk read and write routines.
//
// See the other headers in the folder for prototypes.
//

#ifndef GEX_MESSAGES_H
#define GEX_MESSAGES_H

#include "platform.h"
#include "sched_queue.h"
#include "TinyFrame.h"

/**
 * Supported message types (TF_TYPE)
 */
enum Message_Types_ {
    // General, low level
    MSG_SUCCESS = 0x00, //!< Generic success response; used by default in all responses; payload is transaction-specific
    MSG_PING = 0x01,    //!< Ping request (or response), used to test connection
    MSG_ERROR = 0x02,   //!< Generic failure response (when a request fails to execute)

    MSG_BULK_READ_OFFER = 0x03,  //!< Offer of data to read. Payload: u32 total len
    MSG_BULK_READ_POLL = 0x04,   //!< Request to read a previously announced chunk.
    MSG_BULK_WRITE_OFFER = 0x05, //!< Offer to receive data in a write transaction. Payload: u32 max size, u32 max chunk
    MSG_BULK_DATA = 0x06,        //!< Writing a chunk, or sending a chunk to master.
    MSG_BULK_END = 0x07,         //!< Bulk transfer is done, no more data to read or write. Recipient shall check total len and discard it on mismatch. There could be a checksum ...
    MSG_BULK_ABORT = 0x08,       //!< Discard the ongoing transfer

    // Unit messages
    MSG_UNIT_REQUEST = 0x10,     //!< Command addressed to a particular unit
    MSG_UNIT_REPORT = 0x11,      //!< Spontaneous report from a unit

    // System messages
    MSG_LIST_UNITS = 0x20,       //!< Get all active unit call-signs, types and names
    MSG_INI_READ = 0x21,         //!< Read the ini file via bulk
    MSG_INI_WRITE = 0x22,        //!< Write the ini file via bulk
    MSG_PERSIST_CFG = 0x23,      //!< Write current settings to Flash (the equivalent of replacing the lock jumper)
};

/** The shared USB-serial TinyFrame instance */
extern TinyFrame *comm;

// those must be after the enum because it's used in the header files
#include "msg_responses.h"
#include "msg_bulkread.h"
#include "msg_bulkwrite.h"
#include "event_reports.h"

/**
 * Initialize TinyFrame and set up listeners
 */
void comm_init(void);

#endif //GEX_MESSAGES_H
