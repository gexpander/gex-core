//
// Created by MightyPork on 2018/01/29.
//

#include "comm/messages.h"
#include "unit_base.h"
#include "unit_1wire.h"

// 1WIRE master
#define OW_INTERNAL
#include "_ow_internal.h"
#include "_ow_low_level.h"

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
static void U1WIRE_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->port_name = pp_char(pp);
    priv->pin_number = pp_u8(pp);
    if (version >= 1) {
        priv->parasitic = pp_bool(pp);
    }
}

/** Write to a binary buffer for storing in Flash */
static void U1WIRE_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 1); // version

    pb_char(pb, priv->port_name);
    pb_u8(pb, priv->pin_number);
    pb_bool(pb, priv->parasitic);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
static error_t U1WIRE_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "pin")) {
        suc = parse_pin(value, &priv->port_name, &priv->pin_number);
    }
    else if (streq(key, "parasitic")) {
        priv->parasitic = str_parse_yn(value, &suc);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
static void U1WIRE_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Data pin");
    iw_entry(iw, "pin", "%c%d", priv->port_name,  priv->pin_number);

    iw_comment(iw, "Parasitic (bus-powered) mode");
    iw_entry(iw, "parasitic", str_yn(priv->parasitic));
}

// ------------------------------------------------------------------------

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
static void U1WIRE_TimerCb(TimerHandle_t xTimer)
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

/** Allocate data structure and set defaults */
static error_t U1WIRE_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    // the timer is not started until needed
    priv->busyWaitTimer = xTimerCreate("1w_tim", // name
                                       750,      // interval (will be changed when starting it)
                                       true,     // periodic (we use this only for the polling variant, the one-shot will stop the timer in the CB)
                                       unit,     // user data
                                       U1WIRE_TimerCb); // callback

    if (priv->busyWaitTimer == NULL) return E_OUT_OF_MEM;

    // some defaults
    priv->pin_number = 0;
    priv->port_name = 'A';
    priv->parasitic = false;

    return E_SUCCESS;
}

/** Finalize unit set-up */
static error_t U1WIRE_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    // --- Parse config ---
    priv->ll_pin = hw_pin2ll(priv->pin_number, &suc);
    priv->port = hw_port2periph(priv->port_name, &suc);
    Resource rsc = hw_pin2resource(priv->port_name, priv->pin_number, &suc);
    if (!suc) return E_BAD_CONFIG;

    // --- Claim resources ---
    TRY(rsc_claim(unit, rsc));

    // --- Init hardware ---
    LL_GPIO_SetPinMode(priv->port, priv->ll_pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(priv->port, priv->ll_pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(priv->port, priv->ll_pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinPull(priv->port, priv->ll_pin, LL_GPIO_PULL_UP); // pull-up for OD state

    return E_SUCCESS;
}

/** Tear down the unit */
static void U1WIRE_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // Release all resources
    rsc_teardown(unit);

    // Delete the software timer
    assert_param(pdPASS == xTimerDelete(priv->busyWaitTimer, 1000));

    // Free memory
    free_ck(unit->data);
}

// ------------------------------------------------------------------------


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
static error_t U1WIRE_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
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
    .preInit = U1WIRE_preInit,
    .cfgLoadBinary = U1WIRE_loadBinary,
    .cfgWriteBinary = U1WIRE_writeBinary,
    .cfgLoadIni = U1WIRE_loadIni,
    .cfgWriteIni = U1WIRE_writeIni,
    // Init
    .init = U1WIRE_init,
    .deInit = U1WIRE_deInit,
    // Function
    .handleRequest = U1WIRE_handleRequest,
};
