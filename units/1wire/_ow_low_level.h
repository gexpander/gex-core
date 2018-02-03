//
// Created by MightyPork on 2018/02/01.
//

#ifndef GEX_F072_OW_LOW_LEVEL_H
#define GEX_F072_OW_LOW_LEVEL_H

#ifndef OW_INTERNAL
#error bad include!
#endif

#include "platform.h"
#include "unit_base.h"
#include "_ow_low_level.h"

/**
 * Compute a 1-wire type checksum.
 * If the buffer includes the checksum, the result should be 0.
 *
 * (this function may be used externally, or you can delete the implementation
 * from the c file if another implementation is already available)
 *
 * @param[in] buf - buffer of bytes to verify
 * @param[in] len - buffer length
 * @return checksum
 */
uint8_t ow_checksum(const uint8_t *buf, uint32_t len);

/**
 * Reset the 1-wire bus
 */
bool ow_reset(Unit *unit);

/**
 * Write a bit to the 1-wire bus
 */
void ow_write_bit(Unit *unit, bool bit);

/**
 * Read a bit from the 1-wire bus
 */
bool ow_read_bit(Unit *unit);

/**
 * Write a byte to the 1-wire bus
 */
void ow_write_u8(Unit *unit, uint8_t byte);

/**
 * Write a halfword to the 1-wire bus
 */
void ow_write_u16(Unit *unit, uint16_t halfword);

/**
 * Write a word to the 1-wire bus
 */
void ow_write_u32(Unit *unit, uint32_t word);

/**
 * Write a doubleword to the 1-wire bus
 */
void ow_write_u64(Unit *unit, uint64_t dword);

/**
 * Read a byte form the 1-wire bus
 */
uint8_t ow_read_u8(Unit *unit);

/**
 * Read a halfword form the 1-wire bus
 */
uint16_t ow_read_u16(Unit *unit);

/**
 * Read a word form the 1-wire bus
 */
uint32_t ow_read_u32(Unit *unit);

/**
 * Read a doubleword form the 1-wire bus
 */
uint64_t ow_read_u64(Unit *unit);

#endif //GEX_F072_OW_LOW_LEVEL_H
