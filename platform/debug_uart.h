//
// Created by MightyPork on 2017/12/15.
//
// Setup and routines for debug printing.
// The actual printf-like functions used for debug printing are in debug.h
//

#ifndef GEX_DEBUG_UART_H
#define GEX_DEBUG_UART_H

/**
 * Pre-init the debug uart
 *
 * Set up the peripheral for printing, do not claim resources yet because the
 * registry is not initialized
 */
void DebugUart_PreInit(void);

/**
 * Finalize the init (claim resources)
 */
void DebugUart_Init(void);

/**
 * Write some bytes via the debug USART
 */
void debug_write(const char *buf, uint16_t len);

#endif //GEX_DEBUG_UART_H
