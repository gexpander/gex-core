//
// Created by MightyPork on 2017/12/15.
//

#ifndef GEX_DEBUG_UART_H
#define GEX_DEBUG_UART_H

void DebugUart_PreInit(void);
void DebugUart_Init(void);

void debug_write(const char *buf, uint16_t len);

#endif //GEX_DEBUG_UART_H
