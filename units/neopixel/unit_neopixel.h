//
// Created by MightyPork on 2017/11/25.
//
// NeoPixel RGB LED strip bit-banged output.
// The nanosecond timing is derived from the AHB clock speed.
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
error_t UU_Npx_Clear(Unit *unit);

/**
 * Load the strip with packed bytes R,G,B.
 *
 * @param unit
 * @param packed_rgb - bytes to load
 * @param nbytes - number of bytes, must be count*3
 * @return success
 */
error_t UU_Npx_Load(Unit *unit, const uint8_t *packed_rgb, uint32_t nbytes);

/**
 * Load from 32-bit numbers
 * @param unit
 * @param bytes
 * @param nbytes
 * @param order_bgr
 * @param zero_before
 * @return success
 */
error_t UU_Npx_Load32(Unit *unit, const uint8_t *bytes, uint32_t nbytes, bool order_bgr, bool zero_before);

/**
 * Get number of pixels on the strip
 *
 * @param unit
 * @param count - destination for the count value
 * @return success
 */
error_t UU_Npx_GetCount(Unit *unit, uint16_t *count);

#endif //U_NEOPIXEL_H
