//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_i2c.h"

#define I2C_INTERNAL
#include "_i2c_internal.h"

static void i2c_reset(struct priv *priv)
{
    LL_I2C_Disable(priv->periph);
    HAL_Delay(1);
    LL_I2C_Enable(priv->periph);
}

static error_t i2c_wait_until_flag(struct priv *priv, uint32_t flag, bool stop_state)
{
    uint32_t t_start = HAL_GetTick();
    while (((priv->periph->ISR & flag)!=0) != stop_state) {
        if (HAL_GetTick() - t_start > 10) {
            i2c_reset(priv);
            return E_HW_TIMEOUT;
        }
    }
    return E_SUCCESS;
}

error_t UU_I2C_Write(Unit *unit, uint16_t addr, const uint8_t *bytes, uint32_t bcount)
{
    CHECK_TYPE(unit, &UNIT_I2C);

    struct priv *priv = unit->data;

    uint8_t addrsize = (uint8_t) (((addr & 0x8000) == 0) ? 7 : 10);
    addr &= 0x3FF;
    uint32_t ll_addrsize = (addrsize == 7) ? LL_I2C_ADDRSLAVE_7BIT : LL_I2C_ADDRSLAVE_10BIT;
    if (addrsize == 7) addr <<= 1; // 7-bit address must be shifted to left for LL to use it correctly

    TRY(i2c_wait_until_flag(priv, I2C_ISR_BUSY, 0));

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
            TRY(i2c_wait_until_flag(priv, I2C_ISR_TXIS, 1));
            uint8_t byte = *bytes++;
            LL_I2C_TransmitData8(priv->periph, byte);
        }
    }

    TRY(i2c_wait_until_flag(priv, I2C_ISR_STOPF, 1));
    LL_I2C_ClearFlag_STOP(priv->periph);
    return E_SUCCESS;
}

error_t UU_I2C_Read(Unit *unit, uint16_t addr, uint8_t *dest, uint32_t bcount)
{
    CHECK_TYPE(unit, &UNIT_I2C);

    struct priv *priv = unit->data;

    uint8_t addrsize = (uint8_t) (((addr & 0x8000) == 0) ? 7 : 10);
    addr &= 0x3FF;
    uint32_t ll_addrsize = (addrsize == 7) ? LL_I2C_ADDRSLAVE_7BIT : LL_I2C_ADDRSLAVE_10BIT;
    if (addrsize == 7) addr <<= 1; // 7-bit address must be shifted to left for LL to use it correctly

    TRY(i2c_wait_until_flag(priv, I2C_ISR_BUSY, 0));

    bool first = true;
    while (bcount > 0) {
        if (!first) {
            TRY(i2c_wait_until_flag(priv, I2C_ISR_TCR, 1));
        }

        uint8_t chunk_remain = (uint8_t) ((bcount > 255) ? 255 : bcount); // if more than 255, first chunk is 255
        LL_I2C_HandleTransfer(priv->periph, addr, ll_addrsize, chunk_remain,
                              (bcount > 255) ? LL_I2C_MODE_RELOAD : LL_I2C_MODE_AUTOEND, // Autoend if this is the last chunk
                              first ? LL_I2C_GENERATE_START_READ : LL_I2C_GENERATE_NOSTARTSTOP); // no start/stop condition if we're continuing
        first = false;
        bcount -= chunk_remain;

        for (; chunk_remain > 0; chunk_remain--) {
            TRY(i2c_wait_until_flag(priv, I2C_ISR_RXNE, 1));

            uint8_t byte = LL_I2C_ReceiveData8(priv->periph);
            *dest++ = byte;
        }
    }

    TRY(i2c_wait_until_flag(priv, I2C_ISR_STOPF, 1));
    LL_I2C_ClearFlag_STOP(priv->periph);
    return E_SUCCESS;
}

error_t UU_I2C_ReadReg(Unit *unit, uint16_t addr, uint8_t regnum, uint8_t *dest, uint32_t width)
{
    TRY(UU_I2C_Write(unit, addr, &regnum, 1));
    TRY(UU_I2C_Read(unit, addr, dest, width));
    return E_SUCCESS;
}

error_t UU_I2C_WriteReg(Unit *unit, uint16_t addr, uint8_t regnum, const uint8_t *bytes, uint32_t width)
{
    CHECK_TYPE(unit, &UNIT_I2C);

    // we have to insert the address first - needs a buffer (XXX realistically the buffer needs 1-4 bytes + addr)
    PayloadBuilder pb = pb_start((uint8_t*)unit_tmp512, UNIT_TMP_LEN, NULL);
    pb_u8(&pb, regnum);
    pb_buf(&pb, bytes, width);

    TRY(UU_I2C_Write(unit, addr, (uint8_t *) unit_tmp512, pb_length(&pb)));
    return E_SUCCESS;
}
