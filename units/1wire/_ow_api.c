//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_1wire.h"

// 1WIRE master
#define OW_INTERNAL
#include "_ow_internal.h"
#include "_ow_commands.h"
#include "_ow_low_level.h"

/* Check presence of any devices on the bus */
error_t UU_1WIRE_CheckPresence(Unit *unit, bool *presence)
{
    CHECK_TYPE(unit, &UNIT_1WIRE);
    // reset
    *presence = ow_reset(unit);
    return E_SUCCESS;
}

/* Read address of a lone device on the bus */
error_t UU_1WIRE_ReadAddress(Unit *unit, uint64_t *address)
{
    CHECK_TYPE(unit, &UNIT_1WIRE);
    *address = 0;
    if (!ow_reset(unit)) return E_HW_TIMEOUT;

    // command
    ow_write_u8(unit, OW_ROM_READ);

    // read the ROM code
    *address = ow_read_u64(unit);

    const uint8_t *addr_as_bytes = (void*)address;
    if (0 != ow_checksum(addr_as_bytes, 8)) {
        *address = 0;
        return E_CHECKSUM_MISMATCH; // checksum mismatch
    }
    return E_SUCCESS;
}

/* Write bytes to a device */
error_t UU_1WIRE_Write(Unit *unit, uint64_t address, const uint8_t *buff, uint32_t len)
{
    CHECK_TYPE(unit, &UNIT_1WIRE);
    if (!ow_reset(unit)) return E_HW_TIMEOUT;

    // MATCH_ROM+addr, or SKIP_ROM
    if (address != 0) {
        ow_write_u8(unit, OW_ROM_MATCH);
        ow_write_u64(unit, address);
    } else {
        ow_write_u8(unit, OW_ROM_SKIP);
    }

    // write the payload;
    for (uint32_t i = 0; i < len; i++) {
        ow_write_u8(unit, *buff++);
    }
    return E_SUCCESS;
}

/* Write a request to a device and read a response */
error_t UU_1WIRE_Read(Unit *unit, uint64_t address,
                      const uint8_t *request_buff, uint32_t request_len,
                      uint8_t *response_buff, uint32_t response_len, bool check_crc)
{
    CHECK_TYPE(unit, &UNIT_1WIRE);
    if (!ow_reset(unit)) return E_HW_TIMEOUT;

    uint8_t *rb = response_buff;

    // MATCH_ROM+addr, or SKIP_ROM
    if (address != 0) {
        ow_write_u8(unit, OW_ROM_MATCH);
        ow_write_u64(unit, address);
    } else {
        ow_write_u8(unit, OW_ROM_SKIP);
    }

    // write the payload;
    for (uint32_t i = 0; i < request_len; i++) {
        ow_write_u8(unit, *request_buff++);
    }

    // read the requested number of bytes
    for (uint32_t i = 0; i < response_len; i++) {
        *rb++ = ow_read_u8(unit);
    }

    if (check_crc) {
        if (0 != ow_checksum(response_buff, response_len)) {
            return E_CHECKSUM_MISMATCH;
        }
    }
    return E_SUCCESS;
}

/* Perform the search algorithm (start or continue) */
error_t UU_1WIRE_Search(Unit *unit, bool with_alarm, bool restart,
                        uint64_t *buffer, uint32_t capacity, uint32_t *real_count,
                        bool *have_more)
{
    CHECK_TYPE(unit, &UNIT_1WIRE);
    struct priv *priv = unit->data;

    if (restart) {
        uint8_t search_cmd = (uint8_t) (with_alarm ? OW_ROM_ALM_SEARCH : OW_ROM_SEARCH);
        ow_search_init(unit, search_cmd, true);
    }

    *real_count = ow_search_run(unit, (ow_romcode_t *) buffer, capacity);

    // resolve the code
    switch (priv->searchState.status) {
        case OW_SEARCH_MORE:
            *have_more = priv->searchState.status == OW_SEARCH_MORE;

        case OW_SEARCH_DONE:
            return E_SUCCESS;

        case OW_SEARCH_FAILED:
            return priv->searchState.error;
    }

    return E_INTERNAL_ERROR;
}
