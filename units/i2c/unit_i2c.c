//
// Created by MightyPork on 2018/01/02.
//

#include "comm/messages.h"
#include "unit_base.h"
#include "utils/avrlibc.h"
#include "unit_i2c.h"

// I2C master
#define I2C_INTERNAL
#include "_i2c_internal.h"

enum PinCmd_ {
    CMD_WRITE = 0,
    CMD_READ = 1,
    CMD_WRITE_REG = 2,
    CMD_READ_REG = 3,
};

/** Handle a request message */
static error_t UI2C_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    uint16_t addr;
    uint32_t len;
    uint8_t regnum;
    uint32_t size;

    // NOTE: 10-bit addresses must have the highest bit set to 1 for indication (0x8000 | addr)

    switch (command) {
        /** Write byte(s) - addr:u16, byte(s)  */
        case CMD_WRITE:
            addr = pp_u16(pp);
            const uint8_t *bb = pp_tail(pp, &len);

            return UU_I2C_Write(unit, addr, bb, len);

        /** Read byte(s) - addr:u16, len:u16 */
        case CMD_READ:
            addr = pp_u16(pp);
            len = pp_u16(pp);

            TRY(UU_I2C_Read(unit, addr, (uint8_t *) unit_tmp512, len));
            com_respond_buf(frame_id, MSG_SUCCESS, (uint8_t *) unit_tmp512, len);
            return E_SUCCESS;

        /** Read register(s) - addr:u16, reg:u8, size:u16 */
        case CMD_READ_REG:;
            addr = pp_u16(pp);
            regnum = pp_u8(pp); // register number
            size = pp_u16(pp); // total number of bytes to read (allows use of auto-increment)

            TRY(UU_I2C_ReadReg(unit, addr, regnum, (uint8_t *) unit_tmp512, size));
            com_respond_buf(frame_id, MSG_SUCCESS, (uint8_t *) unit_tmp512, size);
            return E_SUCCESS;

        /** Write a register - addr:u16, reg:u8, byte(s) */
        case CMD_WRITE_REG:
            addr = pp_u16(pp);
            regnum = pp_u8(pp); // register number
            const uint8_t *tail = pp_tail(pp, &size);

            return UU_I2C_WriteReg(unit, addr, regnum, tail, size);

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_I2C = {
    .name = "I2C",
    .description = "I2C master",
    // Settings
    .preInit = UI2C_preInit,
    .cfgLoadBinary = UI2C_loadBinary,
    .cfgWriteBinary = UI2C_writeBinary,
    .cfgLoadIni = UI2C_loadIni,
    .cfgWriteIni = UI2C_writeIni,
    // Init
    .init = UI2C_init,
    .deInit = UI2C_deInit,
    // Function
    .handleRequest = UI2C_handleRequest,
};
