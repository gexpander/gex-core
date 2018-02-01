//
// Created by MightyPork on 2018/01/29.
//

#ifndef GEX_F072_OW_INTERNAL_H
#define GEX_F072_OW_INTERNAL_H

#ifndef OW_INTERNAL
#error bad include!
#endif

#include "_ow_search.h"

/** Private data structure */
struct priv {
    char port_name;
    uint8_t pin_number;
    bool parasitic;

    GPIO_TypeDef *port;
    uint32_t ll_pin;

    TimerHandle_t busyWaitTimer; // timer used to wait for ds1820 measurement completion
    bool busy; // flag used when the timer is running
    uint32_t busyStart;
    TF_ID busyRequestId;
    struct ow_search_state searchState;
};

// Prototypes

#endif //GEX_F072_OW_INTERNAL_H
