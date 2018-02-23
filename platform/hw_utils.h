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
#include "ll_extension.h"

/**
 * Convert pin number to LL driver bitfield for working with the pin.
 *
 * @param pin_number - number 0..15
 * @param suc - set to false on failure, left unchanged on success.
 * @return LL_GPIO_PIN_x
 */
uint32_t hw_pin2ll(uint8_t pin_number, bool *suc);

/**
 * Convert port name to peripheral instance
 *
 * @param port_name - char 'A'..'Z'
 * @param suc - set to false on failure, left unchanged on success.
 * @return instance
 */
GPIO_TypeDef *hw_port2periph(char port_name, bool *suc);

/**
 * Convert a pin resource to it's LL lib values
 *
 * @param[in]  rsc - resource to process
 * @param[out] port - output port
 * @param[out] llpin - output LL pin mask
 * @return success
 */
bool hw_pinrsc2ll(Resource rsc, GPIO_TypeDef **port, uint32_t *llpin) __attribute__((warn_unused_result));

/**
 * Spread packed port pins using a mask
 *
 * @param packed - packed bits, right-aligned (eg. 0b1111)
 * @param mask - positions of the bits (eg. 0x8803)
 * @return - bits spread to their positions (always counting from right)
 */
uint32_t pinmask_spread_32(uint32_t packed, uint32_t mask);

/** Spread packed port pins using a mask - 16-bit version */
static inline uint16_t pinmask_spread(uint16_t packed, uint16_t mask)
{
    return (uint16_t) pinmask_spread_32(packed, mask);
}

/**
 * Pack spread port pins using a mask
 *
 * @param spread - bits in a port register (eg. 0xFF02)
 * @param mask - mask of the bits we want to pack (eg. 0x8803)
 * @return - packed bits, right aligned (eg. 0b1110)
 */
uint32_t pinmask_pack_32(uint32_t spread, uint32_t mask);

/** Pack spread port pins using a mask - 16-bit version */
static inline uint16_t pinmask_pack(uint32_t spread, uint32_t mask)
{
    return (uint16_t) pinmask_pack_32(spread, mask);
}

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
bool hw_solve_timer(uint32_t base_freq, uint32_t required_freq, bool is16bit,
                    uint16_t *presc, uint32_t *count, float *real_freq) __attribute__((warn_unused_result));

#define hw_wait_while(call, timeout) \
    do { \
        uint32_t _ts = HAL_GetTick(); \
        while (1 == (call)) { \
            if (HAL_GetTick() - _ts > (timeout)) { \
                trap("Timeout"); \
            } \
        } \
    } while (0)

#define hw_wait_until(call, timeout) hw_wait_while(!(call), (timeout))

#endif //GEX_PIN_UTILS_H
