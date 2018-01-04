//
// Created by MightyPork on 2018/01/02.
//

#include "comm/messages.h"
#include "unit_base.h"
#include "utils/avrlibc.h"
#include "unit_i2c.h"

// I2C master

/** Private data structure */
struct priv {
    uint8_t periph_num; //!< 1 or 2
    bool anf;    //!< Enable analog noise filter
    uint8_t dnf; //!< Enable digital noise filter (1-15 ... max spike width)
    uint8_t speed; //!< 0 - Standard, 1 - Fast, 2 - Fast+

    I2C_TypeDef *periph;

    GPIO_TypeDef *port;
    uint32_t ll_pin_scl;
    uint32_t ll_pin_sda;
};

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
}

/** Write to a binary buffer for storing in Flash */
static void UI2C_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_u8(pb, priv->periph_num);
    pb_bool(pb, priv->anf);
    pb_u8(pb, priv->dnf);
    pb_u8(pb, priv->speed);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
static bool UI2C_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "device")) {
        priv->periph_num = (uint8_t) avr_atoi(value);
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
        return false;
    }

    return suc;
}

/** Generate INI file section for the unit */
static void UI2C_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Peripheral number (I2Cx)");
    iw_entry(iw, "device", "%d", (int)priv->periph_num);

    iw_comment(iw, "Speed: 1-Standard, 2-Fast, 3-Fast+");
    iw_entry(iw, "speed", "%d", (int)priv->speed);

    iw_comment(iw, "Analog noise filter enable (Y,N)");
    iw_entry(iw, "analog-filter", "%s", str_yn(priv->anf));

    iw_comment(iw, "Digital noise filter bandwidth (0-15)");
    iw_entry(iw, "digital-filter", "%d", (int)priv->dnf);
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static bool UI2C_preInit(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv), &suc);
    CHECK_SUC();

    // some defaults
    priv->periph_num = 1;
    priv->speed = 1;
    priv->anf = true;
    priv->dnf = 0;

    return true;
}

/** Finalize unit set-up */
static bool UI2C_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (!(priv->periph_num >= 1 && priv->periph_num <= 2)) {
        unit->status = E_BAD_CONFIG;
        dbg("!! Bad I2C periph");
        return false;
    }

    if (!(priv->speed >= 1 && priv->speed <= 3)) {
        unit->status = E_BAD_CONFIG;
        dbg("!! Bad I2C speed");
        return false;
    }

    if (priv->dnf > 15) {
        unit->status = E_BAD_CONFIG;
        dbg("!! Bad I2C DNF bw");
        return false;
    }

    // assign and claim the peripheral
    if (priv->periph_num == 1) {
        suc = rsc_claim(unit, R_I2C1);
        CHECK_SUC();
        priv->periph = I2C1;
    } else {
        suc = rsc_claim(unit, R_I2C2);
        CHECK_SUC();
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
    portname = 'B';

    if (priv->periph_num == 1) {
        pin_scl = 8;
        pin_sda = 9;
    } else {
        pin_scl = 10;
        pin_sda = 12;
    }

    af_i2c = LL_GPIO_AF_1;
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
    Resource r_sda = pin2resource(portname, pin_sda, &suc);
    Resource r_scl = pin2resource(portname, pin_scl, &suc);
    CHECK_SUC();
    rsc_claim(unit, r_sda);
    rsc_claim(unit, r_scl);
    CHECK_SUC();

    GPIO_TypeDef *port = port2periph(portname, &suc);
    uint32_t ll_pin_scl = pin2ll(pin_scl, &suc);
    uint32_t ll_pin_sda = pin2ll(pin_sda, &suc);
    CHECK_SUC();

    // configure AF
    if (pin_scl < 8) LL_GPIO_SetAFPin_0_7(port, ll_pin_scl, af_i2c);
    else LL_GPIO_SetAFPin_8_15(port, ll_pin_scl, af_i2c);

    if (pin_sda < 8) LL_GPIO_SetAFPin_0_7(port, ll_pin_sda, af_i2c);
    else LL_GPIO_SetAFPin_8_15(port, ll_pin_sda, af_i2c);

    LL_GPIO_SetPinMode(port, ll_pin_scl, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(port, ll_pin_sda, LL_GPIO_MODE_ALTERNATE);

    // set as OpenDrain (this may not be needed - TODO check)
    LL_GPIO_SetPinOutputType(port, ll_pin_scl, LL_GPIO_OUTPUT_OPENDRAIN);
    LL_GPIO_SetPinOutputType(port, ll_pin_sda, LL_GPIO_OUTPUT_OPENDRAIN);


    if (priv->periph_num == 1) {
        __HAL_RCC_I2C1_CLK_ENABLE();
    } else {
        __HAL_RCC_I2C2_CLK_ENABLE();
    }

    /* Disable the selected I2Cx Peripheral */
    LL_I2C_Disable(priv->periph);
    LL_I2C_ConfigFilters(priv->periph,
                         priv->anf ? LL_I2C_ANALOGFILTER_ENABLE
                                   : LL_I2C_ANALOGFILTER_DISABLE,
                         priv->dnf);

    LL_I2C_SetTiming(priv->periph, timing);
    //LL_I2C_DisableClockStretching(priv->periph);
    LL_I2C_Enable(priv->periph);

    LL_I2C_DisableOwnAddress1(priv->periph); // OA not used
    LL_I2C_SetMode(priv->periph, LL_I2C_MODE_I2C); // not using SMBus

    return true;
}

/** Tear down the unit */
static void UI2C_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init the pins & peripheral only if inited correctly
    if (unit->status == E_SUCCESS) {
        LL_I2C_DeInit(priv->periph);

        if (priv->periph_num == 1) {
            __HAL_RCC_I2C1_CLK_DISABLE();
        } else {
            __HAL_RCC_I2C2_CLK_DISABLE();
        }

        LL_GPIO_SetPinMode(priv->port, priv->ll_pin_sda, LL_GPIO_MODE_ANALOG);
        LL_GPIO_SetPinMode(priv->port, priv->ll_pin_scl, LL_GPIO_MODE_ANALOG);
    }

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free(unit->data);
    unit->data = NULL;
}

// ------------------------------------------------------------------------

enum PinCmd_ {
    CMD_WRITE = 0,
    CMD_READ = 1,
    CMD_WRITE_REG = 2,
    CMD_READ_REG = 3,
};

static void i2c_reset(struct priv *priv)
{
    LL_I2C_Disable(priv->periph);
    HAL_Delay(1);
    LL_I2C_Enable(priv->periph);
}

static bool i2c_wait_until_flag(struct priv *priv, uint32_t flag, bool stop_state)
{
    uint32_t t_start = HAL_GetTick();
    while (((priv->periph->ISR & flag)!=0) != stop_state) {
        if (HAL_GetTick() - t_start > 10) {
            i2c_reset(priv);
            return false;
        }
    }
    return true;
}

bool UU_I2C_Write(Unit *unit, uint16_t addr, const uint8_t *bytes, uint32_t bcount)
{
    struct priv *priv = unit->data;

    uint8_t addrsize = (uint8_t) (((addr & 0x8000) == 0) ? 7 : 10);
    addr &= 0x3FF;
    uint32_t ll_addrsize = (addrsize == 7) ? LL_I2C_ADDRSLAVE_7BIT : LL_I2C_ADDRSLAVE_10BIT;
    if (addrsize == 7) addr <<= 1; // 7-bit address must be shifted to left for LL to use it correctly

    if (!i2c_wait_until_flag(priv, I2C_ISR_BUSY, 0)) {
        dbg("BUSY TOUT");
        return false;
    }

    bool first = true;
    while (bcount > 0) {
        uint32_t len = bcount;
        uint32_t chunk_remain = (uint8_t) ((len > 255) ? 255 : len); // if more than 255, first chunk is 255
        LL_I2C_HandleTransfer(priv->periph, addr, ll_addrsize, chunk_remain,
                              (len > 255) ? LL_I2C_MODE_RELOAD : LL_I2C_MODE_AUTOEND, // Autoend if this is the last chunk
                              first ? LL_I2C_GENERATE_START_WRITE : LL_I2C_GENERATE_NOSTARTSTOP); // no start/stop condition if we're continuing
        first = false;
        bcount -= chunk_remain;

        for (; chunk_remain > 0; chunk_remain--) {
            if (!i2c_wait_until_flag(priv, I2C_ISR_TXIS, 1)) {
                dbg("TXIS TOUT, remain %d", (int)chunk_remain);
                return false;
            }
            uint8_t byte = *bytes++;
            LL_I2C_TransmitData8(priv->periph, byte);
        }
    }

    if (!i2c_wait_until_flag(priv, I2C_ISR_STOPF, 1)) {
        dbg("STOPF TOUT");
        return false;
    }
    LL_I2C_ClearFlag_STOP(priv->periph);
    return true;
}

bool UU_I2C_Read(Unit *unit, uint16_t addr, uint8_t *dest, uint32_t bcount)
{
    struct priv *priv = unit->data;

    uint8_t addrsize = (uint8_t) (((addr & 0x8000) == 0) ? 7 : 10);
    addr &= 0x3FF;
    uint32_t ll_addrsize = (addrsize == 7) ? LL_I2C_ADDRSLAVE_7BIT : LL_I2C_ADDRSLAVE_10BIT;
    if (addrsize == 7) addr <<= 1; // 7-bit address must be shifted to left for LL to use it correctly

    if (!i2c_wait_until_flag(priv, I2C_ISR_BUSY, 0)) {
        dbg("BUSY TOUT");
        return false;
    }

    bool first = true;
    uint32_t n = 0;
    while (bcount > 0) {
        if (!first) {
            if (!i2c_wait_until_flag(priv, I2C_ISR_TCR, 1)) {
                dbg("TCR TOUT");
                return false;
            }
        }

        uint8_t chunk_remain = (uint8_t) ((bcount > 255) ? 255 : bcount); // if more than 255, first chunk is 255
        LL_I2C_HandleTransfer(priv->periph, addr, ll_addrsize, chunk_remain,
                              (bcount > 255) ? LL_I2C_MODE_RELOAD : LL_I2C_MODE_AUTOEND, // Autoend if this is the last chunk
                              first ? LL_I2C_GENERATE_START_READ : LL_I2C_GENERATE_NOSTARTSTOP); // no start/stop condition if we're continuing
        first = false;
        bcount -= chunk_remain;

        for (; chunk_remain > 0; chunk_remain--) {
            if (!i2c_wait_until_flag(priv, I2C_ISR_RXNE, 1)) {
                dbg("RXNE TOUT");
                return false;
            }

            uint8_t byte = LL_I2C_ReceiveData8(priv->periph);
            *dest++ = byte;
        }
    }

    if (!i2c_wait_until_flag(priv, I2C_ISR_STOPF, 1)) {
        dbg("STOPF TOUT");
        return false;
    }
    LL_I2C_ClearFlag_STOP(priv->periph);
    return true;
}

bool UU_I2C_ReadReg(Unit *unit, uint16_t addr, uint8_t regnum, uint8_t *dest, uint32_t width)
{
    if (!UU_I2C_Write(unit, addr, &regnum, 1)) {
        return false;
    }

    // we read the register as if it was a unsigned integer
    if (!UU_I2C_Read(unit, addr, (uint8_t *) unit_tmp512, width)) {
        return false;
    }

    return true;
}

bool UU_I2C_WriteReg(Unit *unit, uint16_t addr, uint8_t regnum, const uint8_t *bytes, uint32_t width)
{
    PayloadBuilder pb = pb_start((uint8_t*)unit_tmp512, 512, NULL);
    pb_u8(&pb, regnum);
    pb_buf(&pb, bytes, width);

    if (!UU_I2C_Write(unit, addr, (uint8_t *) unit_tmp512, pb_length(&pb))) {
        return false;
    }

    return true;
}

/** Handle a request message */
static bool UI2C_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint16_t addr;
    uint32_t len;

    // 10-bit address has the highest bit set to 1 to indicate this

    uint8_t regnum; // register number
    uint32_t size; // register width

    switch (command) {
        case CMD_WRITE:
            addr = pp_u16(pp);
            const uint8_t *bb = pp_tail(pp, &len);

            if (!UU_I2C_Write(unit, addr, bb, len)) {
                com_respond_err(frame_id, "TX FAIL");
                return false;
            }
            break;

        case CMD_READ:
            addr = pp_u16(pp);
            len = pp_u16(pp);

            if (!UU_I2C_Read(unit, addr, (uint8_t *) unit_tmp512, len)) {
                com_respond_err(frame_id, "RX FAIL");
                return false;
            }
            com_respond_buf(frame_id, MSG_SUCCESS, (uint8_t *) unit_tmp512, len);
            break;

        case CMD_READ_REG:;
            addr = pp_u16(pp);
            regnum = pp_u8(pp); // register number
            size = pp_u8(pp); // total number of bytes to read (allows use of auto-increment)

            if (!UU_I2C_ReadReg(unit, addr, regnum, (uint8_t *) unit_tmp512, size)) {
                com_respond_err(frame_id, "READ REG FAIL");
                return false;
            }
            com_respond_buf(frame_id, MSG_SUCCESS, (uint8_t *) unit_tmp512, size);
            break;

        case CMD_WRITE_REG:
            addr = pp_u16(pp);
            regnum = pp_u8(pp); // register number

            const uint8_t *tail = pp_tail(pp, &size);

            if (!UU_I2C_WriteReg(unit, addr, regnum, tail, size)) {
                com_respond_err(frame_id, "WRITE REG FAIL");
                return false;
            }
            break;

        default:
            com_respond_bad_cmd(frame_id);
            return false;
    }

    return true;
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
