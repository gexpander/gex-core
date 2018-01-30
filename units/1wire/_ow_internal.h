//
// Created by MightyPork on 2018/01/29.
//

#ifndef GEX_F072_OW_INTERNAL_H
#define GEX_F072_OW_INTERNAL_H

#ifndef OW_INTERNAL
#error bad include!
#endif

#include "_ow_commands.h"

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
};

// Prototypes

/**
 * Reset the 1-wire bus
 */
bool ow_reset(Unit *unit);

/**
 * Write a bit to the 1-wire bus
 */
void ow_write_bit(Unit *unit, bool bit);

/**
 * Read a bit from the 1-wire bus
 */
bool ow_read_bit(Unit *unit);

/**
 * Write a byte to the 1-wire bus
 */
void ow_write_u8(Unit *unit, uint8_t byte);

/**
 * Write a halfword to the 1-wire bus
 */
void ow_write_u16(Unit *unit, uint16_t halfword);

/**
 * Write a word to the 1-wire bus
 */
void ow_write_u32(Unit *unit, uint32_t word);

/**
 * Write a doubleword to the 1-wire bus
 */
void ow_write_u64(Unit *unit, uint64_t dword);

/**
 * Read a byte form the 1-wire bus
 */
uint8_t ow_read_u8(Unit *unit);

/**
 * Read a halfword form the 1-wire bus
 */
uint16_t ow_read_u16(Unit *unit);

/**
 * Read a word form the 1-wire bus
 */
uint32_t ow_read_u32(Unit *unit);

/**
 * Read a doubleword form the 1-wire bus
 */
uint64_t ow_read_u64(Unit *unit);

#endif //GEX_F072_OW_INTERNAL_H
