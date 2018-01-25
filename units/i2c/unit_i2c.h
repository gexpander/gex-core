//
// Created by MightyPork on 2018/01/02.
//
// I2C master unit
//

#ifndef GEX_F072_UNIT_I2C_H
#define GEX_F072_UNIT_I2C_H

#include "unit.h"

extern const UnitDriver UNIT_I2C;

// Unit-to-Unit API

/**
 * Raw write to I2C
 *
 * @param unit - I2C unit
 * @param addr - device address (set highest bit if address is 10-bit)
 * @param bytes - bytes to write
 * @param bcount - byte count
 * @return success
 */
error_t UU_I2C_Write(Unit *unit, uint16_t addr, const uint8_t *bytes, uint32_t bcount);

/**
 * Raw read from I2C
 *
 * @param unit - I2C unit
 * @param addr - device address (set highest bit if address is 10-bit)
 * @param dest - buffer for read bytes
 * @param bcount - byte count
 * @return success
 */
error_t UU_I2C_Read(Unit *unit, uint16_t addr, uint8_t *dest, uint32_t bcount);

/**
 * Read one or more registers from a I2C register-based device with auto-increment.
 *
 * @param unit - I2C unit
 * @param addr - device address (set highest bit if address is 10-bit)
 * @param regnum - first register number
 * @param dest - destination buffer
 * @param width - register width (or multiple consecutive registers total size)
 * @return success
 */
error_t UU_I2C_ReadReg(Unit *unit, uint16_t addr, uint8_t regnum, uint8_t *dest, uint32_t width);

/**
 * Write a register value
 *
 * @param unit - I2C unit
 * @param addr - device address (set highest bit if address is 10-bit)
 * @param regnum - register number
 * @param bytes - register bytes (use &byte) if just one
 * @param width - register width (number of bytes)
 * @return success
 */
error_t UU_I2C_WriteReg(Unit *unit, uint16_t addr, uint8_t regnum, const uint8_t *bytes, uint32_t width);

/**
 * Write a 8-bit register value
 *
 * @param unit - I2C unit
 * @param addr - device address (set highest bit if address is 10-bit)
 * @param regnum - register number
 * @param value - byte to write to the register
 * @return success
 */
static inline error_t UU_I2C_WriteReg8(Unit *unit, uint16_t addr, uint8_t regnum, uint8_t value)
{
    return UU_I2C_WriteReg(unit, addr, regnum, &value, 1);
}

#endif //GEX_F072_UNIT_I2C_H
