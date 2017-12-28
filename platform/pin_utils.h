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
uint32_t pin2ll(uint8_t pin_number, bool *suc);

/**
 * Convert pin name and number to a resource enum
 *
 * @param port_name - char 'A'..'Z'
 * @param pin_number - number 0..15
 * @param suc - set to false on failure, left unchanged on success
 * @return the resource, or R_NONE
 */
Resource pin2resource(char port_name, uint8_t pin_number, bool *suc);

/**
 * Convert port name to peripheral instance
 *
 * @param port_name - char 'A'..'Z'
 * @param suc - set to false on failure, left unchanged on success.
 * @return instance
 */
GPIO_TypeDef *port2periph(char port_name, bool *suc);

/** Parse a pin name PA0 or A0 to port name and pin number */
bool parse_pin(const char *str, char *targetName, uint8_t *targetNumber);

/** Parse a port name */
bool parse_port(const char *value, char *targetName);

/** Parse a list of pin numbers with ranges and commans/semicolons to a bitmask */
uint16_t parse_pinmask(const char *value, bool *suc);

/** Convert a pin bitmask to the ASCII format understood by str_parse_pinmask() */
char * str_pinmask(uint16_t pins, char *buffer);

/**
 * Spread packed port pins using a mask
 *
 * @param packed - packed bits, right-aligned (eg. 0b1111)
 * @param mask - positions of the bits (eg. 0x8803)
 * @return - bits spread to their positions (always counting from right)
 */
uint16_t port_spread(uint16_t packed, uint16_t mask);

/**
 * Pack spread port pins using a mask
 *
 * @param spread - bits in a port register (eg. 0xFF02)
 * @param mask - mask of the bits we want to pack (eg. 0x8803)
 * @return - packed bits, right aligned (eg. 0b1110)
 */
uint16_t port_pack(uint16_t spread, uint16_t mask);

#endif //GEX_PIN_UTILS_H
