#pragma once

/* Includes ------------------------------------------------------------------*/

#include "main.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/**
 * @brief Compose an RGB color.
 * @param r, g, b - components 0xFF
 * @returns integer 0xRRGGBB
 */
#define rgb(r, g, b) (((0xFF & (r)) << 16) | ((0xFF & (g)) << 8) | (0xFF & (b)))

/* Get components */
#define rgb_r(rgb) (uint8_t)(((rgb) >> 16) & 0xFF)
#define rgb_g(rgb) (uint8_t)(((rgb) >> 8) & 0xFF)
#define rgb_b(rgb) (uint8_t)((rgb) & 0xFF)

typedef union {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    uint32_t num;
} xrgb_t;

/* Exported functions --------------------------------------------------------*/


/**
 * @brief Set color of multiple chained RGB leds
 *
 * @param port
 * @param ll_pin
 * @param rgbs - array of colors (0xRRGGBB)
 * @param count - number of pixels
 */
void ws2812_load(GPIO_TypeDef *port, uint32_t ll_pin, uint32_t *rgbs, uint32_t count);

/**
 * Load RGBs from a packed byte stream
 *
 * @param port
 * @param ll_pin
 * @param rgbs - packed R,G,B, R,G,B, ... array
 * @param count - number of pixels (triplets)
 */
void ws2812_load_raw(GPIO_TypeDef *port, uint32_t ll_pin, uint8_t *rgbs, uint32_t count);

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
 * @param bigendian - big endian ordering
 */
void ws2812_load_sparse(GPIO_TypeDef *port, uint32_t ll_pin, uint8_t *rgbs, uint32_t count, bool bigendian);
