//
// Created by MightyPork on 2018/01/29.
//

#include "comm/messages.h"
#include "unit_base.h"
#include "utils/avrlibc.h"
#include "unit_1wire.h"

// 1WIRE master

/** Private data structure */
struct priv {
    char port_name;
    uint8_t pin_number;

    GPIO_TypeDef *port;
    uint32_t ll_pin;
};

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
static bool ow_reset(Unit *unit)
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
static void ow_write(Unit *unit, bool bit)
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
static bool ow_read(Unit *unit)
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
static void ow_write_u8(Unit *unit, uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        ow_write(unit, 0 != (byte & (1 << i)));
    }
}

/**
 * Write a halfword to the 1-wire bus
 */
static void ow_write_u16(Unit *unit, uint16_t halfword)
{
    ow_write_u8(unit, (uint8_t) (halfword & 0xFF));
    ow_write_u8(unit, (uint8_t) ((halfword >> 8) & 0xFF));
}

/**
 * Write a word to the 1-wire bus
 */
static void ow_write_u32(Unit *unit, uint32_t word)
{
    ow_write_u16(unit, (uint16_t) (word));
    ow_write_u16(unit, (uint16_t) (word >> 16));
}

/**
 * Write a doubleword to the 1-wire bus
 */
static void ow_write_u64(Unit *unit, uint64_t dword)
{
    ow_write_u32(unit, (uint32_t) (dword));
    ow_write_u32(unit, (uint32_t) (dword >> 32));
}

/**
 * Read a byte form the 1-wire bus
 */
static uint8_t ow_read_u8(Unit *unit)
{
    uint8_t buf = 0;
    for (int i = 0; i < 8; i++) {
        buf <<= 1;
        buf |= 1 & ow_read(unit);
    }
    return buf;
}

/**
 * Read a halfword form the 1-wire bus
 */
static uint16_t ow_read_u16(Unit *unit)
{
    uint16_t acu = 0;
    acu |= ow_read_u8(unit);
    acu |= ow_read_u8(unit) << 8;
    return acu;
}

/**
 * Read a word form the 1-wire bus
 */
static uint32_t ow_read_u32(Unit *unit)
{
    uint32_t acu = 0;
    acu |= ow_read_u16(unit);
    acu |= (uint32_t)ow_read_u16(unit) << 16;
    return acu;
}

/**
 * Read a doubleword form the 1-wire bus
 */
static uint64_t ow_read_u64(Unit *unit)
{
    uint64_t acu = 0;
    acu |= ow_read_u32(unit);
    acu |= (uint64_t)ow_read_u32(unit) << 32;
    return acu;
}

// ------------------------------------------------------------------------

#define OW_ROM_SEARCH     0xF0
#define OW_ROM_READ       0x33
#define OW_ROM_MATCH      0x55
#define OW_ROM_SKIP       0xCC
#define OW_ROM_ALM_SEARCH 0xEC

#define OW_DS1820_CONVERT_T     0x44
#define OW_DS1820_WRITE_SCRATCH 0x4E
#define OW_DS1820_READ_SCRATCH  0xBE
#define OW_DS1820_COPY_SCRATCH  0x48
#define OW_DS1820_RECALL_E2     0xB8
#define OW_DS1820_READ_SUPPLY   0xB4

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
            ow_pull_high(unit);

            osDelay(750);
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
