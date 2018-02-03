//
// Created by MightyPork on 2018/02/01.
//

#include "platform.h"

#define OW_INTERNAL
#include "_ow_checksum.h"

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
