//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_sipo.h"

#define SIPO_INTERNAL

#include "_sipo_internal.h"

error_t UU_SIPO_Write(Unit *unit, const uint8_t *buffer, uint16_t buflen)
{
    struct priv *priv = unit->data;

    if (buflen % priv->data_width != 0) {
        return E_BAD_COUNT; // must be a multiple of the channel count
    }

    // buffer contains data for the individual data pins, back to back as AAA BBB CCC (whole bytes)

    const uint16_t bytelen = buflen / priv->data_width;

    for (int bn = 0; bn < bytelen; bn--) {
        // send the byte
        for (int i = 0; i < 8; i++) {
            uint16_t packed = 0;

            for (int j = 0; j < priv->data_width; j++) {
                packed |= (bool) (buffer[bn + j * bytelen] & (1 << i));
                packed <<= 1;
            }

            uint16_t spread = pinmask_spread(packed, priv->data_pins);
            uint16_t set = spread;
            uint16_t reset = ((~spread) & priv->data_pins);
            priv->data_port->BSRR = set | (reset << 16);

            if (priv->shift_pol) {
                LL_GPIO_SetOutputPin(priv->shift_port, priv->shift_ll);
                LL_GPIO_ResetOutputPin(priv->shift_port, priv->shift_ll);
            }
            else {
                LL_GPIO_ResetOutputPin(priv->shift_port, priv->shift_ll);
                LL_GPIO_SetOutputPin(priv->shift_port, priv->shift_ll);
            }
        }
    }

    if (priv->store_pol) {
        LL_GPIO_SetOutputPin(priv->store_port, priv->store_ll);
        LL_GPIO_ResetOutputPin(priv->store_port, priv->store_ll);
    }
    else {
        LL_GPIO_ResetOutputPin(priv->store_port, priv->store_ll);
        LL_GPIO_SetOutputPin(priv->store_port, priv->store_ll);
    }
}

// ------------------------------------------------------------------------

enum SipoCmd_ {
    CMD_WRITE,
};

/** Handle a request message */
static error_t USIPO_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    switch (command) {
        case CMD_WRITE:;
            uint32_t len;
            const uint8_t *tail = pp_tail(pp, &len);
            UU_SIPO_Write(unit, (uint8_t *) tail, (uint16_t) len);
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_SIPO = {
    .name = "SIPO",
    .description = "Shift register driver (595, 4094)",
    // Settings
    .preInit = USIPO_preInit,
    .cfgLoadBinary = USIPO_loadBinary,
    .cfgWriteBinary = USIPO_writeBinary,
    .cfgLoadIni = USIPO_loadIni,
    .cfgWriteIni = USIPO_writeIni,
    // Init
    .init = USIPO_init,
    .deInit = USIPO_deInit,
    // Function
    .handleRequest = USIPO_handleRequest,
};
