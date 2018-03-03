//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_pwmdim.h"

#define PWMDIM_INTERNAL
#include "_pwmdim_internal.h"

// ------------------------------------------------------------------------

enum PwmSimpleCmd_ {
    CMD_SET_FREQUENCY = 0,
    CMD_SET_DUTY = 1,
    CMD_STOP = 2,
    CMD_START = 3,
};

/** Handle a request message */
static error_t UPWMDIM_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    switch (command) {
        case CMD_SET_FREQUENCY:
            TRY(UPWMDIM_SetFreq(unit, pp_u32(pp)));
            return E_SUCCESS;

        case CMD_SET_DUTY:
            for (; pp_length(pp) > 0;) {
                uint8_t ch = pp_u8(pp);
                uint16_t duty = pp_u16(pp);
                TRY(UPWMDIM_SetDuty(unit, ch, duty));
            }
            return E_SUCCESS;

        case CMD_STOP:
            LL_TIM_DisableCounter(priv->TIMx);
            LL_TIM_SetCounter(priv->TIMx, 0);
            return E_SUCCESS;

        case CMD_START:
            LL_TIM_EnableCounter(priv->TIMx);
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Simple PWM dimming output */
const UnitDriver UNIT_PWMDIM = {
    .name = "PWMDIM",
    .description = "Simple PWM output",
    // Settings
    .preInit = UPWMDIM_preInit,
    .cfgLoadBinary = UPWMDIM_loadBinary,
    .cfgWriteBinary = UPWMDIM_writeBinary,
    .cfgLoadIni = UPWMDIM_loadIni,
    .cfgWriteIni = UPWMDIM_writeIni,
    // Init
    .init = UPWMDIM_init,
    .deInit = UPWMDIM_deInit,
    // Function
    .handleRequest = UPWMDIM_handleRequest,
};
