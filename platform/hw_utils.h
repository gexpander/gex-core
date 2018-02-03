//
// Created by MightyPork on 2017/12/08.
//
// Utilities for parsing pins from settings to LL and resources,
// GPIO init, AF config and some LL driver extensions
//

#ifndef GEX_PIN_UTILS_H
#define GEX_PIN_UTILS_H

#include "platform.h"
#include "resources.h"

extern const uint32_t LL_SYSCFG_EXTI_PORTS[PORTS_COUNT];
extern const uint32_t LL_SYSCFG_EXTI_LINES[16];
extern GPIO_TypeDef * const GPIO_PERIPHS[PORTS_COUNT];
extern const uint32_t LL_GPIO_PINS[16];
extern const uint32_t LL_EXTI_LINES[16];

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
 * This is the downto variant (15..0)
 *
 * @param pins - sparse pin map
 * @param buffer - output string buffer
 * @return the output buffer
 */
char * pinmask2str(uint16_t pins, char *buffer);

/**
 * Convert a pin bitmap to the ASCII format understood by str_parse_pinmask()
 * This is the ascending variant (0..15)
 *
 * @param pins - sparse pin map
 * @param buffer - output string buffer
 * @return the output buffer
 */
char * pinmask2str_up(uint16_t pins, char *buffer);

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
 * Convert spread port pin number to a packed index using a mask
 *
 * eg. with a mask 0b1010 and index 3, the result is 1 (bit 1 of the packed - 0bX0)
 */
uint8_t pinmask_translate(uint16_t mask, uint8_t index);

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
error_t hw_configure_gpio_af(char port_name, uint8_t pin_num, uint32_t ll_af) __attribute__((warn_unused_result));

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
                                 uint32_t ll_mode, uint32_t ll_otype) __attribute__((warn_unused_result));

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

/**
 * Solve a timer/counter's count and prescaller value to meet the desired
 * overflow frequency. The resulting values are the dividing factors;
 * subtract 1 before writing them into the peripheral registers.
 *
 * @param[in] base_freq - the counter's input clock frequency in Hz
 * @param[in] required_freq - desired overflow frequency
 * @param[in] is16bit - limit counter to 16 bits (prescaller is always 16-bit)
 * @param[out] presc - field for storing the computed prescaller value
 * @param[out] count - field for storing the computed counter value
 * @param[out] real_freq - field for storing the computed real frequency
 * @return true on success
 */
bool solve_timer(uint32_t base_freq, uint32_t required_freq, bool is16bit,
                 uint16_t *presc, uint32_t *count, float *real_freq);

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
