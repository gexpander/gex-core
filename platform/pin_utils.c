//
// Created by MightyPork on 2017/11/26.
//

#include <utils/avrlibc.h>
#include "pin_utils.h"
#include "macro.h"

#define PINS_COUNT 16

/** Pin number to LL bitfield mapping */
static const uint32_t ll_pins[PINS_COUNT] = {
    LL_GPIO_PIN_0,
    LL_GPIO_PIN_1,
    LL_GPIO_PIN_2,
    LL_GPIO_PIN_3,
    LL_GPIO_PIN_4,
    LL_GPIO_PIN_5,
    LL_GPIO_PIN_6,
    LL_GPIO_PIN_7,
    LL_GPIO_PIN_8,
    LL_GPIO_PIN_9,
    LL_GPIO_PIN_10,
    LL_GPIO_PIN_11,
    LL_GPIO_PIN_12,
    LL_GPIO_PIN_13,
    LL_GPIO_PIN_14,
    LL_GPIO_PIN_15,
};
COMPILER_ASSERT(16 == ELEMENTS_IN_ARRAY(ll_pins));

/** Port number (A=0) to config struct pointer mapping */
static GPIO_TypeDef * const port_periphs[] = {
    GPIOA,
    GPIOB,
    GPIOC,
    GPIOD,
    GPIOE,
#if PORTS_COUNT>5
    GPIOF,
#endif
#if PORTS_COUNT>6
    GPIOG,
#endif
};
COMPILER_ASSERT(PORTS_COUNT == ELEMENTS_IN_ARRAY(port_periphs));

/** Convert pin number to LL bitfield */
uint32_t pin2ll(uint8_t pin_number, bool *suc)
{
    assert_param(suc != NULL);

    if(pin_number >= PINS_COUNT) {
        dbg("Bad pin: %d", pin_number);
        // TODO proper report
        *suc = false;
        return 0;
    }
    return ll_pins[pin_number];
}

/** Convert port name (A,B,C...) to peripheral struct pointer */
GPIO_TypeDef *port2periph(char port_name, bool *suc)
{
    assert_param(suc != NULL);

    if(port_name < 'A' || port_name >= ('A'+PORTS_COUNT)) {
        dbg("Bad port: %c", port_name); // TODO proper report
        *suc = false;
        return NULL;
    }

    uint8_t num = (uint8_t) (port_name - 'A');
    return port_periphs[num];
}

/** Convert a pin to resource handle */
Resource pin2resource(char port_name, uint8_t pin_number, bool *suc)
{
    assert_param(suc != NULL);

    if(port_name < 'A' || port_name >= ('A'+PORTS_COUNT)) {
        dbg("Bad port: %c", port_name); // TODO proper report
        *suc = false;
        return R_NONE;
    }

    if(pin_number >= PINS_COUNT) {
        dbg("Bad pin: %d", pin_number); // TODO proper report
        *suc = false;
        return R_NONE;
    }

    uint8_t num = (uint8_t) (port_name - 'A');

    return R_PA0 + num*16 + pin_number;
}

/** Parse single pin */
bool parse_pin(const char *value, char *targetName, uint8_t *targetNumber)
{
    // discard leading 'P'
    if (value[0] == 'P') {
        value++;
    }

    size_t len = strlen(value);
    if (len<2||len>3) return false;

    *targetName = (uint8_t) value[0];
    if (!(*targetName >= 'A' && *targetName <= 'H')) return false;

    // lets just hope it's OK
    *targetNumber = (uint8_t) avr_atoi(value + 1);
    return true;
}

/** Parse port name */
bool parse_port(const char *value, char *targetName)
{
    *targetName = (uint8_t) value[0];
    if (!(*targetName >= 'A' && *targetName < 'A' + PORTS_COUNT)) return false;
    return true;
}

/** Parse a list of pin numbers with ranges and commans/semicolons to a bitmask */
uint16_t parse_pinmask(const char *value, bool *suc)
{
    uint32_t bits = 0;
    uint32_t acu = 0;
    bool inrange = false;
    uint32_t rangestart = 0;

    // shortcut if none are set
    if (value[0] == 0) return 0;

    char c;
    do {
        c = *value++;
        if (c == ' ' || c == '\t') {
            // skip
        }
        else if (c >= '0' && c <= '9') {
            acu = acu*10 + (c-'0');
        }
        else if (c == ',' || c == ';' || c == 0) {
            // end of number or range
            if (!inrange) rangestart = acu;

            // swap them if they're in the wrong order
            if (acu < rangestart) {
                uint32_t swp = acu;
                acu = rangestart;
                rangestart = swp;
            }

            for(uint32_t i=rangestart; i<=acu; i++) {
                bits |= 1<<i;
            }

            inrange = false;
            rangestart = 0;
            acu = 0;
        }
        else if (c == '-' || c == ':') {
            rangestart = acu;
            inrange = true;
            acu=0;
        } else {
            *suc = false;
        }
    } while (c != 0);

    if (bits > 0xFFFF) *suc = false;

    return (uint16_t) bits;
}

/** Convert a pin bitmask to the ASCII format understood by str_parse_pinmask() */
char * str_pinmask(uint16_t pins, char *buffer)
{
    char *b = buffer;
    uint32_t start = 0;
    bool on = false;
    bool first = true;

    // shortcut if none are set
    if (pins == 0) {
        buffer[0] = 0;
        return buffer;
    }

    for (int32_t i = 15; i >= -1; i--) {
        bool bit;

        if (i == -1) {
            bit = false;
        } else {
            bit = 0 != (pins & 0x8000);
            pins <<= 1;
        }

        if (bit) {
            if (!on) {
                start = (uint32_t) i;
                on = true;
            }
        } else {
            if (on) {
                if (!first) {
                    b += SPRINTF(b, ",");
                }
                if (start == (uint32_t)(i+1)) {
                    b += SPRINTF(b, "%"PRIu32, start);
                }
                else if (start == (uint32_t)(i+2)) {
                    // exception for 2-long ranges - don't show as range
                    b += SPRINTF(b, "%"PRIu32",%"PRIu32, start, i + 1);
                }
                else {
                    b += SPRINTF(b, "%"PRIu32"-%"PRIu32, start, i + 1);
                }
                first = false;
                on = false;
            }
        }
    }

    return buffer;
}

/** Spread packed port pins using a mask */
uint16_t port_spread(uint16_t packed, uint16_t mask)
{
    uint16_t result = 0;
    uint16_t poke = 1;
    for (int i = 0; i<16; i++) {
        if (mask & (1<<i)) {
            if (packed & poke) {
                result |= 1<<i;
            }
            poke <<= 1;
        }
    }
    return result;
}

/** Pack spread port pins using a mask */
uint16_t port_pack(uint16_t spread, uint16_t mask)
{
    uint16_t result = 0;
    uint16_t poke = 1;
    for (int i = 0; i<16; i++) {
        if (mask & (1<<i)) {
            if (spread & (1<<i)) {
                result |= poke;
            }
            poke <<= 1;
        }
    }
    return result;
}
