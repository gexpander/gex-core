//
// Created by MightyPork on 2017/12/23.
//

#ifndef GEX_F072_MSG_BULKREAD_H
#define GEX_F072_MSG_BULKREAD_H

#ifndef GEX_MESSAGES_H
#error "Include messages.h instead!"
#endif

#include <TinyFrame.h>


#define BULK_LST_TIMEOUT_MS 200

#define BULKREAD_MAX_CHUNK  512 // this is a static buffer


typedef void (*bulkread_data_cb)(uint32_t offset, uint32_t len, uint8_t *buffer);

struct bulk_read {
    TF_ID frame_id;
    bulkread_data_cb read;
    uint32_t len;
    uint32_t offset;
};

void bulkread_start(TinyFrame *tf, struct bulk_read *bulk);


#endif //GEX_F072_MSG_BULKREAD_H
