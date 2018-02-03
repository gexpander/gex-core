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


// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
static void UI2C_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->periph_num = pp_u8(pp);
    priv->anf = pp_bool(pp);
    priv->dnf = pp_u8(pp);
    priv->speed = pp_u8(pp);

    if (version >= 1) {
        priv->remap = pp_u8(pp);
    }
}

/** Write to a binary buffer for storing in Flash */
static void UI2C_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 1); // version

    pb_u8(pb, priv->periph_num);
    pb_bool(pb, priv->anf);
    pb_u8(pb, priv->dnf);
    pb_u8(pb, priv->speed);
    pb_u8(pb, priv->remap);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
static error_t UI2C_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "device")) {
        priv->periph_num = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "remap")) {
        priv->remap = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "analog-filter")) {
        priv->anf = str_parse_yn(value, &suc);
    }
    else if (streq(key, "digital-filter")) {
        priv->dnf = (uint8_t) avr_atoi(value);
    }
    else if (streq(key, "speed")) {
        priv->speed = (uint8_t) avr_atoi(value);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
static void UI2C_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Peripheral number (I2Cx)");
    iw_entry(iw, "device", "%d", (int)priv->periph_num);

    iw_comment(iw, "Pin mappings (SCL,SDA)");
#if GEX_PLAT_F072_DISCOVERY
    iw_comment(iw, " I2C1: (0) B6,B7    (1) B8,B9");
    iw_comment(iw, " I2C2: (0) B10,B11  (1) B13,B14");
#elif GEX_PLAT_F103_BLUEPILL
    #error "NO IMPL"
#elif GEX_PLAT_F303_DISCOVERY
    #error "NO IMPL"
#elif GEX_PLAT_F407_DISCOVERY
    #error "NO IMPL"
#else
    #error "BAD PLATFORM!"
#endif
    iw_entry(iw, "remap", "%d", (int)priv->remap);

    iw_cmt_newline(iw);
    iw_comment(iw, "Speed: 1-Standard, 2-Fast, 3-Fast+");
    iw_entry(iw, "speed", "%d", (int)priv->speed);

    iw_comment(iw, "Analog noise filter enable (Y,N)");
    iw_entry(iw, "analog-filter", "%s", str_yn(priv->anf));

    iw_comment(iw, "Digital noise filter bandwidth (0-15)");
    iw_entry(iw, "digital-filter", "%d", (int)priv->dnf);
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static error_t UI2C_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    // some defaults
    priv->periph_num = 1;
    priv->speed = 1;
    priv->anf = true;
    priv->dnf = 0;

    return E_SUCCESS;
}

/** Finalize unit set-up */
static error_t UI2C_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (!(priv->periph_num >= 1 && priv->periph_num <= 2)) {
        dbg("!! Bad I2C periph"); // TODO report
        return E_BAD_CONFIG;
    }

    if (!(priv->speed >= 1 && priv->speed <= 3)) {
        dbg("!! Bad I2C speed");
        return E_BAD_CONFIG;
    }

    if (priv->dnf > 15) {
        dbg("!! Bad I2C DNF bw");
        return E_BAD_CONFIG;
    }

    // assign and claim the peripheral
    if (priv->periph_num == 1) {
        TRY(rsc_claim(unit, R_I2C1));
        priv->periph = I2C1;
    } else {
        TRY(rsc_claim(unit, R_I2C2));
        priv->periph = I2C2;
    }

    // This is written for F072, other platforms will need adjustments

    char portname;
    uint8_t pin_scl;
    uint8_t pin_sda;
    uint32_t af_i2c;
    uint32_t timing; // magic constant from CubeMX

#if GEX_PLAT_F072_DISCOVERY
    // scl - 6 or 8 for I2C1, 10 for I2C2
    // sda - 7 or 9 for I2C1, 11 for I2C2
    if (priv->periph_num == 1) {
        // I2C1
        if (priv->remap == 0) {
            af_i2c = LL_GPIO_AF_1;
            portname = 'B';
            pin_scl = 6;
            pin_sda = 7;
        } else if (priv->remap == 1) {
            af_i2c = LL_GPIO_AF_1;
            portname = 'B';
            pin_scl = 8;
            pin_sda = 9;
        } else {
            return E_BAD_CONFIG;
        }
    } else {
        // I2C2
        if (priv->remap == 0) {
            af_i2c = LL_GPIO_AF_1;
            portname = 'B';
            pin_scl = 10;
            pin_sda = 11;
        } else if (priv->remap == 1) {
            af_i2c = LL_GPIO_AF_5;
            portname = 'B';
            pin_scl = 13;
            pin_sda = 14;
        } else {
            return E_BAD_CONFIG;
        }
    }

    if (priv->speed == 1)
        timing = 0x00301D2B; // Standard
    else if (priv->speed == 2)
        timing = 0x0000020B; // Fast
    else
        timing = 0x00000001; // Fast+

#elif GEX_PLAT_F103_BLUEPILL
    #error "NO IMPL"
#elif GEX_PLAT_F303_DISCOVERY
    #error "NO IMPL"
#elif GEX_PLAT_F407_DISCOVERY
    #error "NO IMPL"
#else
    #error "BAD PLATFORM!"
#endif

    // first, we have to claim the pins
    TRY(rsc_claim_pin(unit, portname, pin_sda));
    TRY(rsc_claim_pin(unit, portname, pin_scl));

    hw_configure_gpio_af(portname, pin_sda, af_i2c);
    hw_configure_gpio_af(portname, pin_scl, af_i2c);

    hw_periph_clock_enable(priv->periph);

    /* Disable the selected I2Cx Peripheral */
    LL_I2C_Disable(priv->periph);
    LL_I2C_ConfigFilters(priv->periph,
                         (priv->anf ? LL_I2C_ANALOGFILTER_ENABLE : LL_I2C_ANALOGFILTER_DISABLE),
                         priv->dnf);

    LL_I2C_SetTiming(priv->periph, timing);
    //LL_I2C_DisableClockStretching(priv->periph);
    LL_I2C_Enable(priv->periph);

    LL_I2C_DisableOwnAddress1(priv->periph); // OA not used
    LL_I2C_SetMode(priv->periph, LL_I2C_MODE_I2C); // not using SMBus

    return E_SUCCESS;
}

/** Tear down the unit */
static void UI2C_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init the pins & peripheral only if inited correctly
    if (unit->status == E_SUCCESS) {
        assert_param(priv->periph);
        LL_I2C_DeInit(priv->periph);

        hw_periph_clock_disable(priv->periph);
    }

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}

// ------------------------------------------------------------------------

enum PinCmd_ {
    CMD_TEST = 0,
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
        case CMD_TEST:
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
