//
// Created by MightyPork on 2018/01/29.
//

#include "comm/messages.h"
#include "unit_base.h"
#include "utils/avrlibc.h"
#include "unit_1wire.h"

// 1WIRE master
#define OW_INTERNAL
#include "_ow_internal.h"
#include "_ow_commands.h"
#include "_ow_search.h"
#include "_ow_checksum.h"
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

static void U1WIRE_TimerCb(TimerHandle_t xTimer)
{
    Unit *unit = pvTimerGetTimerID(xTimer);
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv->busy);

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

/**
 * Check if there are any units present on the bus
 *
 * @param[in,out] unit
 * @param[out] presence - any devices present
 * @return success
 */
error_t UU_1WIRE_CheckPresence(Unit *unit, bool *presence)
{
    CHECK_TYPE(unit, &UNIT_1WIRE);
    // reset
    *presence = ow_reset(unit);
    return E_SUCCESS;
}

/**
 * Read a device's address (use only with a single device attached)
 *
 * @param[in,out] unit
 * @param[out] address - the device's address, 0 on error or CRC mismatch
 * @return success
 */
error_t UU_1WIRE_ReadAddress(Unit *unit, uint64_t *address)
{
    CHECK_TYPE(unit, &UNIT_1WIRE);
    *address = 0;
    if (!ow_reset(unit)) return E_HW_TIMEOUT;

    // command
    ow_write_u8(unit, OW_ROM_READ);

    // read the ROM code
    *address = ow_read_u64(unit);

    const uint8_t *addr_as_bytes = (void*)address;
    if (0 != ow_checksum(addr_as_bytes, 8)) {
        *address = 0;
        return E_CHECKSUM_MISMATCH; // checksum mismatch
    }
    return E_SUCCESS;
}

/**
 * Write bytes to a device / devices
 *
 * @param[in,out] unit
 * @param[in] address - device address, 0 to skip match (single device or broadcast)
 * @param[in] buff - bytes to write
 * @param[in] len - buffer length
 * @return success
 */
error_t UU_1WIRE_Write(Unit *unit, uint64_t address, const uint8_t *buff, uint32_t len)
{
    CHECK_TYPE(unit, &UNIT_1WIRE);
    if (!ow_reset(unit)) return E_HW_TIMEOUT;

    // MATCH_ROM+addr, or SKIP_ROM
    if (address != 0) {
        ow_write_u8(unit, OW_ROM_MATCH);
        ow_write_u64(unit, address);
    } else {
        ow_write_u8(unit, OW_ROM_SKIP);
    }

    // write the payload;
    for (uint32_t i = 0; i < len; i++) {
        ow_write_u8(unit, *buff++);
    }
    return E_SUCCESS;
}

/**
 * Read bytes from a device / devices, first writing a query
 *
 * @param[in,out] unit
 * @param[in] address - device address, 0 to skip match (single device ONLY!)
 * @param[in] request_buff - bytes to write before reading a response
 * @param[in] request_len - number of bytes to write
 * @param[out] response_buff - buffer for storing the read response
 * @param[in] response_len - number of bytes to read
 * @param[in] check_crc - verify CRC
 * @return success
 */
error_t UU_1WIRE_Read(Unit *unit, uint64_t address,
                      const uint8_t *request_buff, uint32_t request_len,
                      uint8_t *response_buff, uint32_t response_len, bool check_crc)
{
    CHECK_TYPE(unit, &UNIT_1WIRE);
    if (!ow_reset(unit)) return E_HW_TIMEOUT;

    uint8_t *rb = response_buff;

    // MATCH_ROM+addr, or SKIP_ROM
    if (address != 0) {
        ow_write_u8(unit, OW_ROM_MATCH);
        ow_write_u64(unit, address);
    } else {
        ow_write_u8(unit, OW_ROM_SKIP);
    }

    // write the payload;
    for (uint32_t i = 0; i < request_len; i++) {
        ow_write_u8(unit, *request_buff++);
    }

    // read the requested number of bytes
    for (uint32_t i = 0; i < response_len; i++) {
        *rb++ = ow_read_u8(unit);
    }

    if (check_crc) {
        if (0 != ow_checksum(response_buff, response_len)) {
            return E_CHECKSUM_MISMATCH;
        }
    }
    return E_SUCCESS;
}

/**
 * Perform a ROM search operation.
 * The algorithm is on a depth-first search without backtracking,
 * taking advantage of the open-drain topology.
 *
 * This function either starts the search, or continues it.
 *
 * @param[in,out] unit
 * @param[in] with_alarm - true to match only devices in alarm state
 * @param[in] restart - true to restart the search (search from the lowest address)
 * @param[out] buffer - buffer for storing found addresses
 * @param[in] capacity - buffer capacity in address entries (8 bytes)
 * @param[out] real_count - real number of found addresses (for which the CRC matched)
 * @param[out] have_more - flag indicating there are more devices to be found
 * @return success
 */
error_t UU_1WIRE_Search(Unit *unit, bool with_alarm, bool restart,
                        uint64_t *buffer, uint32_t capacity, uint32_t *real_count,
                        bool *have_more)
{
    CHECK_TYPE(unit, &UNIT_1WIRE);
    struct priv *priv = unit->data;
    
    if (restart) {
        uint8_t search_cmd = (uint8_t) (with_alarm ? OW_ROM_ALM_SEARCH : OW_ROM_SEARCH);
        ow_search_init(unit, search_cmd, true);
    }

    *real_count = ow_search_run(unit, (ow_romcode_t *) buffer, capacity);

    // resolve the code
    switch (priv->searchState.status) {
        case OW_SEARCH_MORE:
            *have_more = priv->searchState.status == OW_SEARCH_MORE;

        case OW_SEARCH_DONE:
            return E_SUCCESS;

        case OW_SEARCH_FAILED:
            return priv->searchState.error;
    }

    return E_FAILURE;
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
