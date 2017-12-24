//
// Created by MightyPork on 2017/12/23.
//

#ifndef GEX_F072_MSG_BULKWRITE_H
#define GEX_F072_MSG_BULKWRITE_H

#include <TinyFrame.h>

typedef struct bulk_write BulkWrite;

/**
 * Type of the bulk-write listener. Offset within the data can be retrieved from bulk->offset.
 *
 * If buffer is NULL, this is the last call and the bulk struct must be freed if it was allocated.
 *
 * @param bulk  - a data object passed to the bulkwrite_start() function.
 * @param chunk - destination buffer to fill with the data. NULL if bulk should be freed.
 * @param len   - size of the chunk to write
 */
typedef void (*bulkwrite_data_cb)(BulkWrite *bulk, const uint8_t *chunk, uint32_t len);

/** Bulk read structure */
struct bulk_write {
    TF_ID frame_id;          //!< ID of the requesting frame, used for the entire bulk read session
    bulkwrite_data_cb write; //!< Write callback
    uint32_t len;            //!< Total data length
    void *userdata;          //!< A place for arbitrary userdata

    uint32_t offset;         //!< Internal offset counter, will be set to 0 on start.
};

/**
 * Start a bulk write session
 * @param tf - tinyframe instance
 * @param bulk - bulk write config structure (malloc'd or static)
 */
void bulkwrite_start(TinyFrame *tf, BulkWrite *bulk);


#endif //GEX_F072_MSG_BULKWRITE_H
