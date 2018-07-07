//
// Created by MightyPork on 2018/04/06.
//

#ifndef GEX_CORE_IFACE_USB_H
#define GEX_CORE_IFACE_USB_H

#include "platform.h"

bool iface_usb_ready(void);

/**
 * USB transmit implementation
 *
 * @param buff - buffer to send (can be longer than the buffers)
 * @param len - buffer size
 */
void iface_usb_transmit(const uint8_t *buff, uint32_t len);

#endif //GEX_CORE_IFACE_USB_H
