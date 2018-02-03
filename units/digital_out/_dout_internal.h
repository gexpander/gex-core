//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_DOUT_INTERNAL_H
#define GEX_F072_DOUT_INTERNAL_H

#ifndef DOUT_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Private data structure */
struct priv {
    char port_name;
    uint16_t pins; // pin mask
    uint16_t initial; // initial pin states
    uint16_t open_drain; // open drain pins

    GPIO_TypeDef *port;
};

#endif //GEX_F072_DOUT_INTERNAL_H
