//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_NPX_INTERNAL_H
#define GEX_F072_NPX_INTERNAL_H

#ifndef NPX_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Private data structure */
struct priv {
    char port_name;
    uint8_t pin_number;
    uint16_t pixels;

    uint32_t ll_pin;
    GPIO_TypeDef *port;
};

#endif //GEX_F072_NPX_INTERNAL_H
