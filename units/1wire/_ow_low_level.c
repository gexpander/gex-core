//
// Created by MightyPork on 2018/01/29.
//
// 1-Wire unit low level functions
//

#include "platform.h"

#define OW_INTERNAL
#include "_ow_internal.h"
#include "_ow_low_level.h"

static inline uint8_t crc8_bits(uint8_t data)
{
    uint8_t crc = 0;
    if(data & 1)     crc ^= 0x5e;
    if(data & 2)     crc ^= 0xbc;
    if(data & 4)     crc ^= 0x61;
    if(data & 8)     crc ^= 0xc2;
    if(data & 0x10)  crc ^= 0x9d;
    if(data & 0x20)  crc ^= 0x23;
    if(data & 0x40)  crc ^= 0x46;
    if(data & 0x80)  crc ^= 0x8c;
    return crc;
}

static uint8_t crc8_add(uint8_t cksum, uint8_t byte)
{
    return crc8_bits(byte ^ cksum);
}

uint8_t ow_checksum(const uint8_t *buff, uint32_t len)
{
    uint8_t cksum = 0;
    for(uint32_t i = 0; i < len; i++) {
        cksum = crc8_add(cksum, buff[i]);
    }
    return cksum;
}

// ----------------------------------------------

static inline void ow_pull_high(Unit *unit)
{
    struct priv *priv = unit->data;
    LL_GPIO_SetOutputPin(priv->port, priv->ll_pin);
    LL_GPIO_SetPinMode(priv->port, priv->ll_pin, LL_GPIO_MODE_OUTPUT);
}

static inline void ow_pull_low(Unit *unit)
{
    struct priv *priv = unit->data;
    LL_GPIO_ResetOutputPin(priv->port, priv->ll_pin);
    LL_GPIO_SetPinMode(priv->port, priv->ll_pin, LL_GPIO_MODE_OUTPUT);
}

static inline void ow_release_line(Unit *unit)
{
    struct priv *priv = unit->data;
    LL_GPIO_SetPinMode(priv->port, priv->ll_pin, LL_GPIO_MODE_INPUT);
}

static inline bool ow_sample_line(Unit *unit)
{
    struct priv *priv = unit->data;
    return (bool) LL_GPIO_IsInputPinSet(priv->port, priv->ll_pin);
}

/**
 * Reset the 1-wire bus
 */
bool ow_reset(Unit *unit)
{
    ow_pull_low(unit);
    PTIM_MicroDelay(500);

    bool presence;
    vPortEnterCritical();
    {
        // Strong pull-up (for parasitive power)
        ow_pull_high(unit);
        PTIM_MicroDelay(2);

        // switch to open-drain
        ow_release_line(unit);
        PTIM_MicroDelay(118);

        presence = !ow_sample_line(unit);
    }
    vPortExitCritical();

    PTIM_MicroDelay(130);
    return presence;
}

/**
 * Write a bit to the 1-wire bus
 */
void ow_write_bit(Unit *unit, bool bit)
{
    vPortEnterCritical();
    {
        // start mark
        ow_pull_low(unit);
        PTIM_MicroDelay(2);

        if (bit) ow_pull_high(unit);
        PTIM_MicroDelay(70);

        // Strong pull-up (for parasitive power)
        ow_pull_high(unit);
    }
    vPortExitCritical();

    PTIM_MicroDelay(2);
}

/**
 * Read a bit from the 1-wire bus
 */
bool ow_read_bit(Unit *unit)
{
    bool bit;

    vPortEnterCritical();
    {
        // start mark
        ow_pull_low(unit);
        PTIM_MicroDelay(2);

        ow_release_line(unit);
        PTIM_MicroDelay(20);

        bit = ow_sample_line(unit);
    }
    vPortExitCritical();

    PTIM_MicroDelay(40);

    return bit;
}

/**
 * Write a byte to the 1-wire bus
 */
void ow_write_u8(Unit *unit, uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        ow_write_bit(unit, 0 != (byte & (1 << i)));
    }
}

/**
 * Write a halfword to the 1-wire bus
 */
void ow_write_u16(Unit *unit, uint16_t halfword)
{
    ow_write_u8(unit, (uint8_t) (halfword & 0xFF));
    ow_write_u8(unit, (uint8_t) ((halfword >> 8) & 0xFF));
}

/**
 * Write a word to the 1-wire bus
 */
void ow_write_u32(Unit *unit, uint32_t word)
{
    ow_write_u16(unit, (uint16_t) (word));
    ow_write_u16(unit, (uint16_t) (word >> 16));
}

/**
 * Write a doubleword to the 1-wire bus
 */
void ow_write_u64(Unit *unit, uint64_t dword)
{
    ow_write_u32(unit, (uint32_t) (dword));
    ow_write_u32(unit, (uint32_t) (dword >> 32));
}

/**
 * Read a byte form the 1-wire bus
 */
uint8_t ow_read_u8(Unit *unit)
{
    uint8_t buf = 0;
    for (int i = 0; i < 8; i++) {
        buf |= (1 & ow_read_bit(unit)) << i;
    }
    return buf;
}

/**
 * Read a halfword form the 1-wire bus
 */
uint16_t ow_read_u16(Unit *unit)
{
    uint16_t acu = 0;
    acu |= ow_read_u8(unit);
    acu |= ow_read_u8(unit) << 8;
    return acu;
}

/**
 * Read a word form the 1-wire bus
 */
uint32_t ow_read_u32(Unit *unit)
{
    uint32_t acu = 0;
    acu |= ow_read_u16(unit);
    acu |= (uint32_t)ow_read_u16(unit) << 16;
    return acu;
}

/**
 * Read a doubleword form the 1-wire bus
 */
uint64_t ow_read_u64(Unit *unit)
{
    uint64_t acu = 0;
    acu |= ow_read_u32(unit);
    acu |= (uint64_t)ow_read_u32(unit) << 32;
    return acu;
}
