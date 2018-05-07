#ifndef WS2812_H
#define WS2812_H

#include "platform.h"

/**
 * Load RGBs from a packed byte stream
 *
 * @param port
 * @param ll_pin
 * @param rgbs - packed R,G,B, R,G,B, ... array
 * @param count - number of pixels (triplets)
 */
void ws2812_load_raw(GPIO_TypeDef *port, uint32_t ll_pin, const uint8_t *rgbs, uint32_t count);

/**
 * Load all pixels with BLACK (0,0,0)
 *
 * @param port
 * @param ll_pin
 * @param count - number of pixels
 */
void ws2812_clear(GPIO_TypeDef *port, uint32_t ll_pin, uint32_t count);

/**
 * Load from a stream of 32-bit numbers (4th or 1st byte skipped)
 * @param port
 * @param ll_pin
 * @param rgbs - payload
 * @param count - number of pixels
 * @param order_bgr - B,G,R colors, false - R,G,B
 * @param zero_before - insert padding byte before colors, false - after
 */
void ws2812_load_sparse(GPIO_TypeDef *port, uint32_t ll_pin, const uint8_t *rgbs, uint32_t count,
                        bool order_bgr, bool zero_before);

#endif //WS2812_H
