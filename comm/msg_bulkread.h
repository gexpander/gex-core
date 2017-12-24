//
// Created by MightyPork on 2017/12/23.
//

#ifndef GEX_F072_MSG_BULKREAD_H
#define GEX_F072_MSG_BULKREAD_H

#ifndef GEX_MESSAGES_H
#error "Include messages.h instead!"
#endif

#include <TinyFrame.h>

typedef struct bulk_read BulkRead;

/**
 * Type of the bulk-read listener. Offset within the data can be retrieved from bulk->offset.
 *
 * If buffer is NULL, this is the last call and the bulk struct must be freed if it was allocated.
 *
 * @param bulk - a data object passed to the bulkread_start() function.
 * @param chunk - size of the chunk to read
 * @param buffer - destination buffer to fill with the data. NULL if bulk should be freed.
 */
typedef void (*bulkread_data_cb)(BulkRead *bulk, uint32_t chunk, uint8_t *buffer);

/** Bulk read structure */
struct bulk_read {
    TF_ID frame_id;        //!< ID of the requesting frame, used for the entire bulk read session
    bulkread_data_cb read; //!< Read callback
    uint32_t len;          //!< Total data length
    void *userdata;        //!< A place for arbitrary userdata

    uint32_t offset;       //!< Internal offset counter, will be set to 0 on start.
};

/**
 * Start a bulk read session
 * @param tf - tinyframe instance
 * @param bulk - bulk read config structure (malloc'd or static)
 */
void bulkread_start(TinyFrame *tf, BulkRead *bulk);


#endif //GEX_F072_MSG_BULKREAD_H
