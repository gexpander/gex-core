//
// Created by MightyPork on 2017/11/25.
//

#ifndef U_NEOPIXEL_H
#define U_NEOPIXEL_H

#include "unit.h"

extern const UnitDriver UNIT_NEOPIXEL;

/**
 * Clear the Neopixel strip
 *
 * @param unit
 * @return success
 */
error_t UU_NEOPIXEL_Clear(Unit *unit);

/**
 * Load the strip with packed bytes R,G,B.
 *
 * @param unit
 * @param packed_rgb - bytes to load
 * @param nbytes - number of bytes, must be count*3
 * @return success
 */
error_t UU_NEOPIXEL_Load(Unit *unit, const uint8_t *packed_rgb, uint32_t nbytes);

/**
 * Load the strip with sparse (uint32_t) colors 0x00RRGGBB as little endian bytes
 * (B, G, R, 0, ...)
 *
 * @param unit
 * @param bytes - bytes to load
 * @param nbytes - number of bytes, must be count*4
 * @return success
 */
error_t UU_NEOPIXEL_LoadU32LE(Unit *unit, const uint8_t *bytes, uint32_t nbytes);

/**
 * Load the strip with sparse (uint32_t) colors 0x00RRGGBB as big endian bytes
 * (0, R, G, B, ...)
 *
 * @param unit
 * @param bytes - bytes to load
 * @param nbytes - number of bytes, must be count*4
 * @return success
 */
error_t UU_NEOPIXEL_LoadU32BE(Unit *unit, const uint8_t *bytes, uint32_t nbytes);

/**
 * Get number of pixels on the strip
 *
 * @param unit
 * @param count - destination for the count value
 * @return success
 */
error_t UU_NEOPIXEL_GetCount(Unit *unit, uint16_t *count);

#endif //U_NEOPIXEL_H
