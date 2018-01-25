//
// Created by MightyPork on 2018/01/02.
//
// USART driver using DMA buffers.
// Provides a wide range of config options and supports driving a RS485 driver.
//
// The implementation is complex and split to multiple files for easier maintenance
//

#ifndef GEX_F072_UNIT_USART_H
#define GEX_F072_UNIT_USART_H

#include "unit.h"

extern const UnitDriver UNIT_USART;

/**
 * Write bytes. This function is asynchronous and does not wait for completion.
 * It blocks until there's space in the Tx buffer for the data.
 *
 * @param unit
 * @param buffer - bytes to send
 * @param len - number of bytes to send
 * @return success
 */
error_t UU_USART_Write(Unit *unit, const uint8_t *buffer, uint32_t len);

/**
 * Write bytes. Same like UU_USART_Write(), except it waits for the transmission
 * to complete after sending the last data.
 *
 * @param unit
 * @param buffer - bytes to send
 * @param len - number of bytes to send
 * @return success
 */
error_t UU_USART_WriteSync(Unit *unit, const uint8_t *buffer, uint32_t len);

#endif //GEX_F072_UNIT_USART_H
