//
// Created by MightyPork on 2017/11/26.
//

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
uint32_t plat_pin2ll(uint8_t pin_number, bool *suc)
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
GPIO_TypeDef *plat_port2periph(char port_name, bool *suc)
{
    assert_param(suc != NULL);

    if(port_name < 'A' || port_name >= ('A'+PORTS_COUNT)) {
        dbg("Bad port: %c", port_name);
        // TODO proper report
        *suc = false;
        return NULL;
    }

    uint8_t num = (uint8_t) (port_name - 'A');
    return port_periphs[num];
}

/** Convert a pin to resource handle */
Resource plat_pin2resource(char port_name, uint8_t pin_number, bool *suc)
{
    assert_param(suc != NULL);

    if(port_name < 'A' || port_name >= ('A'+PORTS_COUNT)) {
        dbg("Bad port: %c", port_name);
        // TODO proper report
        *suc = false;
        return R_NONE;
    }

    if(pin_number >= PINS_COUNT) {
        // TODO proper report
        dbg("Bad pin: %d", pin_number);
        *suc = false;
        return R_NONE;
    }

    uint8_t num = (uint8_t) (port_name - 'A');

    return R_PA0 + num*16 + pin_number;
}
