//
// Created by MightyPork on 2018/01/02.
//
// Dallas 1-Wire master unit
//

#ifndef GEX_F072_UNIT_1WIRE_H
#define GEX_F072_UNIT_1WIRE_H

#include "unit.h"

extern const UnitDriver UNIT_1WIRE;

/**
 * Check if there are any units present on the bus
 *
 * @param[in,out] unit
 * @param[out] presence - any devices present
 * @return success
 */
error_t UU_1WIRE_CheckPresence(Unit *unit, bool *presence);

/**
 * Read a device's address (use only with a single device attached)
 *
 * @param[in,out] unit
 * @param[out] address - the device's address, 0 on error or CRC mismatch
 * @return success
 */
error_t UU_1WIRE_ReadAddress(Unit *unit, uint64_t *address);

/**
 * Write bytes to a device / devices
 *
 * @param[in,out] unit
 * @param[in] address - device address, 0 to skip match (single device or broadcast)
 * @param[in] buff - bytes to write
 * @param[in] len - buffer length
 * @return success
 */
error_t UU_1WIRE_Write(Unit *unit, uint64_t address, const uint8_t *buff, uint32_t len);

/**
 * Read bytes from a device / devices, first writing a query
 *
 * @param[in,out] unit
 * @param[in] address - device address, 0 to skip match (single device ONLY!)
 * @param[in] request_buff - bytes to write before reading a response
 * @param[in] request_len - number of bytes to write
 * @param[out] response_buff - buffer for storing the read response
 * @param[in] response_len - number of bytes to read
 * @param[in] check_crc - verify CRC
 * @return success
 */
error_t UU_1WIRE_Read(Unit *unit, uint64_t address,
                      const uint8_t *request_buff, uint32_t request_len,
                      uint8_t *response_buff, uint32_t response_len, bool check_crc);

/**
 * Perform a ROM search operation.
 * The algorithm is on a depth-first search without backtracking,
 * taking advantage of the open-drain topology.
 *
 * This function either starts the search, or continues it.
 *
 * @param[in,out] unit
 * @param[in] with_alarm - true to match only devices in alarm state
 * @param[in] restart - true to restart the search (search from the lowest address)
 * @param[out] buffer - buffer for storing found addresses
 * @param[in] capacity - buffer capacity in address entries (8 bytes)
 * @param[out] real_count - real number of found addresses (for which the CRC matched)
 * @param[out] have_more - flag indicating there are more devices to be found
 * @return success
 */
error_t UU_1WIRE_Search(Unit *unit, bool with_alarm, bool restart,
                        uint64_t *buffer, uint32_t capacity, uint32_t *real_count,
                        bool *have_more);

#endif //GEX_F072_UNIT_1WIRE_H
