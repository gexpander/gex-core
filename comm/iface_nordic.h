//
// Created by MightyPork on 2018/04/06.
//

#ifndef GEX_F072_IFACE_NORDIC_H
#define GEX_F072_IFACE_NORDIC_H

#include "platform.h"

#if SUPPORT_NRF

void iface_nordic_claim_resources(void);

void iface_nordic_free_resources(void);

void iface_nordic_deinit(void);

bool iface_nordic_init(void);

void iface_nordic_transmit(const uint8_t *buff, uint32_t len);

#endif // SUPPORT_NRF

#endif //GEX_F072_IFACE_NORDIC_H
