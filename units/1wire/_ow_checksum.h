//
// Created by MightyPork on 2018/02/01.
//

#ifndef GEX_F072_OW_CHECKSUM_H
#define GEX_F072_OW_CHECKSUM_H

#include <stdint.h>

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
uint8_t ow_checksum(const uint8_t *buf, uint16_t len);

#endif //GEX_F072_OW_CHECKSUM_H
