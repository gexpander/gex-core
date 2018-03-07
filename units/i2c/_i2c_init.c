//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define I2C_INTERNAL
#include "_i2c_internal.h"


/** Allocate data structure and set defaults */
error_t UI2C_preInit(Unit *unit)
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
error_t UI2C_init(Unit *unit)
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

#if STM32F072xB
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

    TRY(hw_configure_gpio_af(portname, pin_sda, af_i2c));
    TRY(hw_configure_gpio_af(portname, pin_scl, af_i2c));

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
void UI2C_deInit(Unit *unit)
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
