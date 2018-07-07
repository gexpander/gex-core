//
// Created by MightyPork on 2018/04/06.
//

#ifndef GEX_CORE_IFACE_UART_H
#define GEX_CORE_IFACE_UART_H

#include "platform.h"

void com_iface_flush_buffer(void);

void iface_uart_deinit(void);
bool iface_uart_init(void);

void iface_uart_free_resources(void);
void iface_uart_claim_resources(void);

void iface_uart_transmit(const uint8_t *buff, uint32_t len);

#endif //GEX_CORE_IFACE_UART_H
