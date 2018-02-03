//
// Created by MightyPork on 2018/01/29.
//

#include "comm/messages.h"
#include "unit_base.h"
#include "unit_1wire.h"

// 1WIRE master
#define OW_INTERNAL
#include "_ow_internal.h"
#include "_ow_init.h"
#include "_ow_settings.h"
#include "_ow_low_level.h"

/**
 * 1-Wire timer callback, used for the 'wait_ready' function.
 *
 * - In parasitic mode, this is a simple 750ms wait, after which a SUCCESS response is sent.
 * - In 3-wire mode, the callback is fired periodically and performs a Read operation on the bus.
 *   The unit responds with 0 while the operation is ongoing. On receiving 1 a SUCCESS response is sent.
 *   The polling is abandoned after a timeout, sending a TIMEOUT response.
 *
 * @param xTimer
 */
void OW_TimerCb(TimerHandle_t xTimer)
{
    Unit *unit = pvTimerGetTimerID(xTimer);
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv->busy);

    // XXX Possible bug
    // This is run on the timers thread. The TF write functions block on a semaphore.
    // The FreeRTOS documentation warns against blocking in the timers thread,
    // but does not say why. This can be solved by scheduling the response using the job queue.

    if (priv->parasitic) {
        // this is the end of the 750ms measurement time
        goto halt_ok;
    } else {
        bool ready = ow_read_bit(unit);
        if (ready) {
            goto halt_ok;
        }

        uint32_t time = PTIM_GetTime();
        if (time - priv->busyStart > 1000) {
            xTimerStop(xTimer, 100);
            com_respond_error(priv->busyRequestId, E_HW_TIMEOUT);
            priv->busy = false;
        }
    }

    return;
halt_ok:
    xTimerStop(xTimer, 100);
    com_respond_ok(priv->busyRequestId);
    priv->busy = false;
}


enum PinCmd_ {
    CMD_CHECK_PRESENCE = 0, // simply tests that any devices are attached
    CMD_SEARCH_ADDR = 1,    // perform a scan of the bus, retrieving all found device ROMs
    CMD_SEARCH_ALARM = 2,   // like normal scan, but retrieve only devices with alarm
    CMD_SEARCH_CONTINUE = 3, // continue the previously started scan, retrieving more devices
    CMD_READ_ADDR = 4,      // read the ROM code from a single device (for single-device bus)

    CMD_WRITE = 10,    // write multiple bytes using the SKIP_ROM command
    CMD_READ = 11,   // write multiple bytes using a ROM address

    CMD_POLL_FOR_1 = 20,

    CMD_TEST = 100,
};


/** Handle a request message */
static error_t OW_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    bool presence;
    uint64_t addr;
    uint32_t remain;
    const uint8_t *tail;

    if (priv->busy) return E_BUSY;

    bool with_alarm = false;
    bool search_reset = false;

    switch (command) {
        /**
         * This is the delay function for DS1820 measurements.
         *
         * Parasitic: Returns success after the required 750ms
         * Non-parasitic: Returns SUCCESS after device responds '1', HW_TIMEOUT after 1s
         */
        case CMD_POLL_FOR_1:
            // This can't be exposed via the UU API, due to being async
            if (priv->parasitic) {
                assert_param(pdPASS == xTimerChangePeriod(priv->busyWaitTimer, 750, 100));
            } else {
                // every 10 ticks
                assert_param(pdPASS == xTimerChangePeriod(priv->busyWaitTimer, 10, 100));
            }
            assert_param(pdPASS == xTimerStart(priv->busyWaitTimer, 100));
            priv->busy = true;
            priv->busyStart = PTIM_GetTime();
            priv->busyRequestId = frame_id;
            return E_SUCCESS; // We will respond when the timer expires

        case CMD_SEARCH_ALARM:
            with_alarm = true;
            // fall-through
        case CMD_SEARCH_ADDR:
            search_reset = true;
            // fall-through
        case CMD_SEARCH_CONTINUE:;
            uint32_t found_count = 0;
            bool have_more = false;

            TRY(UU_1WIRE_Search(unit, with_alarm, search_reset,
                                (void *) unit_tmp512, UNIT_TMP_LEN/8, &found_count,
                                &have_more));

            // use multipart to avoid allocating extra buffer
            uint8_t status_code = (uint8_t) have_more;
            TF_Msg msg = {
                .frame_id = frame_id,
                .type = MSG_SUCCESS,
                .len = (TF_LEN) (found_count * 8 + 1),
            };
            TF_Respond_Multipart(comm, &msg);
            TF_Multipart_Payload(comm, &status_code, 1);
            // the codes are back-to-back stored inside the buffer, we send it directly
            // (it's already little-endian, as if built by PayloadBuilder)
            TF_Multipart_Payload(comm, (uint8_t *) unit_tmp512, found_count * 8);
            TF_Multipart_Close(comm);
            return E_SUCCESS;

        /** Simply check presence of any devices on the bus. Responds with SUCCESS or HW_TIMEOUT */
        case CMD_CHECK_PRESENCE:
            TRY(UU_1WIRE_CheckPresence(unit, &presence));

            com_respond_u8(frame_id, (uint8_t) presence);
            return E_SUCCESS;

        /** Read address of the single device on the bus - returns u64 */
        case CMD_READ_ADDR:
            TRY(UU_1WIRE_ReadAddress(unit, &addr));

            // build response
            PayloadBuilder pb = pb_start(unit_tmp512, UNIT_TMP_LEN, NULL);
            pb_u64(&pb, addr);
            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        /**
         * Write payload to the bus, no confirmation (unless requested).
         *
         * Payload:
         * - Match variant: addr:u64, rest:write_data
         * - Skip variant:  all:write_data
         */
        case CMD_WRITE:
            addr = pp_u64(pp);
            tail = pp_tail(pp, &remain);
            TRY(UU_1WIRE_Write(unit, addr, tail, remain));
            return E_SUCCESS;

        /**
         * Write and read.
         *
         * Payload:
         * - Match variant: addr:u64, read_len:u16, rest:write_data
         * - Skip variant:  read_len:u16, rest:write_data
         */
        case CMD_READ:;
            addr = pp_u64(pp);
            uint16_t rcount = pp_u16(pp);
            bool test_crc = pp_bool(pp);
            tail = pp_tail(pp, &remain);

            TRY(UU_1WIRE_Read(unit, addr,
                              tail, remain,
                              (uint8_t *) unit_tmp512, rcount,
                              test_crc));

            // build response
            com_respond_buf(frame_id, MSG_SUCCESS, (uint8_t *) unit_tmp512, rcount);
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_1WIRE = {
    .name = "1WIRE",
    .description = "1-Wire master",
    // Settings
    .preInit = OW_preInit,
    .cfgLoadBinary = OW_loadBinary,
    .cfgWriteBinary = OW_writeBinary,
    .cfgLoadIni = OW_loadIni,
    .cfgWriteIni = OW_writeIni,
    // Init
    .init = OW_init,
    .deInit = OW_deInit,
    // Function
    .handleRequest = OW_handleRequest,
};
