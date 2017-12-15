//
// Utilities for parsing pins from settings to LL and resources
//
// Created by MightyPork on 2017/12/08.
//

#ifndef GEX_PIN_UTILS_H
#define GEX_PIN_UTILS_H

#include "platform.h"
#include "resources.h"

/**
 * Convert pin number to LL driver bitfield for working with the pin.
 *
 * @param pin_number - number 0..15
 * @param suc - set to false on failure, left unchanged on success.
 * @return LL_GPIO_PIN_x
 */
uint32_t plat_pin2ll(uint8_t pin_number, bool *suc);

/**
 * Convert pin name and number to a resource enum
 *
 * @param port_name - char 'A'..'Z'
 * @param pin_number - number 0..15
 * @param suc - set to false on failure, left unchanged on success
 * @return the resource, or R_NONE
 */
Resource plat_pin2resource(char port_name, uint8_t pin_number, bool *suc);

/**
 * Convert port name to peripheral instance
 *
 * @param port_name - char 'A'..'Z'
 * @param suc - set to false on failure, left unchanged on success.
 * @return instance
 */
GPIO_TypeDef *plat_port2periph(char port_name, bool *suc);

#endif //GEX_PIN_UTILS_H
