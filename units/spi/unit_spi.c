//
// Created by MightyPork on 2018/01/02.
//
// SPI master with unicast and multicats support, up to 16 slave select lines
//

#include "comm/messages.h"
#include "unit_base.h"
#include "utils/avrlibc.h"
#include "unit_spi.h"

#define SPI_INTERNAL
#include "_spi_internal.h"

// SPI master

enum PinCmd_ {
    CMD_QUERY = 0,
    CMD_MULTICAST = 1,
};

/** Handle a request message */
static error_t USPI_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    uint8_t slave;
    uint16_t slaves;
    uint16_t req_len;
    uint16_t resp_skip;
    uint16_t resp_len;
    const uint8_t *bb;

    uint32_t len;

    switch (command) {
        /** Write and read byte(s) - slave_num:u8, req_len:u16, resp_skip:u16, resp_len:u16, byte(s)  */
        case CMD_QUERY:
            slave = pp_u8(pp);
            resp_skip = pp_u16(pp);
            resp_len = pp_u16(pp);

            bb = pp_tail(pp, &len);

            TRY(UU_SPI_Write(unit, slave,
                             bb, (uint8_t *) unit_tmp512,
                             len, resp_skip, resp_len));

            // no response if we aren't reading
            if (resp_len > 0) {
                com_respond_buf(frame_id, MSG_SUCCESS, (uint8_t *) unit_tmp512, resp_len);
            }
            return E_SUCCESS;

        /** Write byte(s) to multiple slaves - slaves:u16, req_len:u16, byte(s)  */
        case CMD_MULTICAST:
            slaves = pp_u16(pp);

            bb = pp_tail(pp, &len);

            TRY(UU_SPI_Multicast(unit, slaves, bb, len));
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_SPI = {
    .name = "SPI",
    .description = "SPI master",
    // Settings
    .preInit = USPI_preInit,
    .cfgLoadBinary = USPI_loadBinary,
    .cfgWriteBinary = USPI_writeBinary,
    .cfgLoadIni = USPI_loadIni,
    .cfgWriteIni = USPI_writeIni,
    // Init
    .init = USPI_init,
    .deInit = USPI_deInit,
    // Function
    .handleRequest = USPI_handleRequest,
};
