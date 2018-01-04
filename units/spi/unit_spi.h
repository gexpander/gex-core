//
// Created by MightyPork on 2018/01/02.
//

#ifndef GEX_F072_UNIT_SPI_H
#define GEX_F072_UNIT_SPI_H

#include "unit.h"

extern const UnitDriver UNIT_SPI;

// Unit-to-Unit API

/**
 * Raw read/write via SPI.
 * It's possible to simultaneously write and read, or skip bytes in either direction.
 *
 * Example scenarios:
 *
 * req 2, skip 2, read 3
 * |<-- req_len --->|
 * [ write ][ write ] . . . . . . . .
 * . . . . . . . . . [ read ][ read ][ read ]
 * |<-- resp_skip ->|<------ resp_len ----->|
 *
 * req 2, skip 0, read 2
 * |<-- req_len --->|
 * [ write ][ write ]
 * [ read  ][ read  ]
 * |<-- resp_len -->|
 *
 * @param unit - SPI unit
 * @param slave_num - slave number (SS pin index)
 * @param request - request bytes buffer
 * @param response - response bytes buffer
 * @param req_len - number of bytes in the request. Will be right-padded with zeros.
 * @param resp_skip - response bytes to discard before starting to capture them
 * @param resp_len - number of bytes to capture, after discarding resp_skip received bytes
 * @return success
 */
error_t UU_SPI_Write(Unit *unit, uint8_t slave_num,
                     const uint8_t *request,
                     uint8_t *response,
                     uint32_t req_len,
                     uint32_t resp_skip,
                     uint32_t resp_len);

#endif //GEX_F072_UNIT_SPI_H
