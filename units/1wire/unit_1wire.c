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

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
static void U1WIRE_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->port_name = pp_char(pp);
    priv->pin_number = pp_u8(pp);
}

/** Write to a binary buffer for storing in Flash */
static void U1WIRE_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_char(pb, priv->port_name);
    pb_u8(pb, priv->pin_number);
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
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static error_t U1WIRE_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    // some defaults
    priv->pin_number = 0;
    priv->port_name = 'A';

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

    // Free memory
    free_ck(unit->data);
}

// ------------------------------------------------------------------------

enum PinCmd_ {
    CMD_TEST = 0,
};

/** Handle a request message */
static error_t U1WIRE_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    switch (command) {
        /** Write byte(s) - addr:u16, byte(s)  */
        case CMD_TEST:;
            bool presence = ow_reset(unit);
            if (!presence) return E_HW_TIMEOUT;

            ow_write_u8(unit, OW_ROM_SKIP);

            ow_write_u8(unit, OW_DS1820_CONVERT_T);
            while (!ow_read_bit(unit));

            //            osDelay(750);
            // TODO this will be done with an async timer
            // If parasitive power is not used, we could poll and check the status bit

            presence = ow_reset(unit);
            if (!presence) return E_HW_TIMEOUT;
            ow_write_u8(unit, OW_ROM_SKIP);

            ow_write_u8(unit, OW_DS1820_READ_SCRATCH);

            uint16_t temp = ow_read_u16(unit);
            uint16_t threg = ow_read_u16(unit);
            uint16_t reserved = ow_read_u16(unit);
            uint8_t cnt_remain = ow_read_u8(unit);
            uint8_t cnt_per_c = ow_read_u8(unit);
            uint8_t crc = ow_read_u8(unit);
            // TODO check CRC

            PayloadBuilder pb = pb_start(unit_tmp512, UNIT_TMP_LEN, NULL);
            pb_u16(&pb, temp);
            pb_u8(&pb, cnt_remain);
            pb_u8(&pb, cnt_per_c);

            dbg("respond ...");
            com_respond_buf(frame_id, MSG_SUCCESS, pb.start, pb_length(&pb));
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
