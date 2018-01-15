//
// Utilities for parsing pins from settings to LL and resources
//
// Created by MightyPork on 2017/12/08.
//

#ifndef GEX_PIN_UTILS_H
#define GEX_PIN_UTILS_H

#include <stm32f072xb.h>
#include "platform.h"
#include "resources.h"

/**
 * Convert pin number to LL driver bitfield for working with the pin.
 *
 * @param pin_number - number 0..15
 * @param suc - set to false on failure, left unchanged on success.
 * @return LL_GPIO_PIN_x
 */
uint32_t hw_pin2ll(uint8_t pin_number, bool *suc);

/**
 * Convert pin name and number to a resource enum
 *
 * @param port_name - char 'A'..'Z'
 * @param pin_number - number 0..15
 * @param suc - set to false on failure, left unchanged on success
 * @return the resource, or R_NONE
 */
Resource hw_pin2resource(char port_name, uint8_t pin_number, bool *suc);

/**
 * Convert port name to peripheral instance
 *
 * @param port_name - char 'A'..'Z'
 * @param suc - set to false on failure, left unchanged on success.
 * @return instance
 */
GPIO_TypeDef *hw_port2periph(char port_name, bool *suc);

/**
 * Parse a pin name (e.g. PA0 or A0) to port name and pin number
 *
 * @param str - source string
 * @param targetName - output: port name (one character)
 * @param targetNumber - output: pin number 0-15
 * @return success
 */
bool parse_pin(const char *str, char *targetName, uint8_t *targetNumber);

/**
 * Parse a port name (one character) - validates that it's within range
 *
 * @param value - source string
 * @param targetName - output: port name (one character)
 * @return success
 */
bool parse_port_name(const char *value, char *targetName);

/**
 * Parse a list of pin numbers with ranges and commands/semicolons to a bitmask.
 * Supported syntax:
 * - comma separated numbers
 * - numbers connected by dash or colon form a range (can be in any order)
 *
 * @param value - source string
 * @param suc - set to False if parsing failed
 * @return the resulting bitmap
 */
uint16_t parse_pinmask(const char *value, bool *suc);

/**
 * Convert a pin bitmap to the ASCII format understood by str_parse_pinmask()
 *
 * @param pins - sparse pin map
 * @param buffer - output string buffer
 * @return the output buffer
 */
char * pinmask2str(uint16_t pins, char *buffer);

/**
 * Spread packed port pins using a mask
 *
 * @param packed - packed bits, right-aligned (eg. 0b1111)
 * @param mask - positions of the bits (eg. 0x8803)
 * @return - bits spread to their positions (always counting from right)
 */
uint16_t pinmask_spread(uint16_t packed, uint16_t mask);

/**
 * Pack spread port pins using a mask
 *
 * @param spread - bits in a port register (eg. 0xFF02)
 * @param mask - mask of the bits we want to pack (eg. 0x8803)
 * @return - packed bits, right aligned (eg. 0b1110)
 */
uint16_t pinmask_pack(uint16_t spread, uint16_t mask);

/**
 * Set all GPIO resources held by unit to analog.
 * This is a part of unit teardown.
 *
 * @param unit - holding unit
 */
void hw_deinit_unit_pins(Unit *unit);

/**
 * Configure a GPIO pin to alternate function.
 *
 * @param port_name - Port name A-F
 * @param pin_num - Pin number 0-15
 * @param ll_af - LL alternate function constant
 * @return success
 */
error_t hw_configure_gpio_af(char port_name, uint8_t pin_num, uint32_t ll_af);

/**
 * Configure multiple pins using the bitmap pattern
 *
 * @param port_name - port name A-F
 * @param mask - Pins bitmap (0x8002 = pins 1 and 15)
 * @param port_dest - destination pointer for the parsed GPIO Port struct. Can be NULL if the value is not needed.
 * @param ll_mode - LL pin mode (in, out, analog...)
 * @param ll_otype - LL output type (push/pull, opendrain)
 * @return success
 */
error_t hw_configure_sparse_pins(char port_name,
                                 uint16_t mask, GPIO_TypeDef **port_dest,
                                 uint32_t ll_mode, uint32_t ll_otype);

/** Helper struct for defining alternate mappings */
struct PinAF {
    char port;
    uint8_t pin;
    uint8_t af;
};

/**
 * Enable a peripheral clock
 * @param periph - any peripheral
 */
void hw_periph_clock_enable(void *periph);

/**
 * Disable a peripheral clock
 * @param periph - any peripheral
 */
void hw_periph_clock_disable(void *periph);

// ---------- LL extras ------------

static inline bool LL_DMA_IsActiveFlag_G(uint32_t isr_snapshot, uint8_t channel)
{
    return 0 != (isr_snapshot & (DMA_ISR_GIF1 << (uint32_t)((channel-1) * 4)));
}

static inline bool LL_DMA_IsActiveFlag_TC(uint32_t isr_snapshot, uint8_t channel)
{
    return 0 != (isr_snapshot & (DMA_ISR_TCIF1 << (uint32_t)((channel-1) * 4)));
}

static inline bool LL_DMA_IsActiveFlag_HT(uint32_t isr_snapshot, uint8_t channel)
{
    return 0 != (isr_snapshot & (DMA_ISR_HTIF1 << (uint32_t)((channel-1) * 4)));
}

static inline bool LL_DMA_IsActiveFlag_TE(uint32_t isr_snapshot, uint8_t channel)
{
    return 0 != (isr_snapshot & (DMA_ISR_TEIF1 << (uint32_t)((channel-1) * 4)));
}

static inline void LL_DMA_ClearFlag_HT(DMA_TypeDef *DMAx, uint8_t channel)
{
    DMAx->IFCR = (DMA_IFCR_CHTIF1 << (uint32_t)((channel-1) * 4));
}

static inline void LL_DMA_ClearFlag_TC(DMA_TypeDef *DMAx, uint8_t channel)
{
    DMAx->IFCR = (DMA_IFCR_CTCIF1 << (uint32_t)((channel-1) * 4));
}

static inline void LL_DMA_ClearFlag_TE(DMA_TypeDef *DMAx, uint8_t channel)
{
    DMAx->IFCR = (DMA_IFCR_CTEIF1 << (uint32_t)((channel-1) * 4));
}

static inline void LL_DMA_ClearFlags(DMA_TypeDef *DMAx, uint8_t channel)
{
    DMAx->IFCR = (DMA_IFCR_CGIF1 << (uint32_t)((channel-1) * 4));
}

#endif //GEX_PIN_UTILS_H
