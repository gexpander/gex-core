//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_dac.h"

#define DAC_INTERNAL
#include "_dac_internal.h"

/**
 * Re-configure the
 * @param unit
 */
void UDAC_Reconfigure(Unit *unit)
{
    struct priv *priv = unit->data;

    // back-up IT state
    bool IT_enabled = (bool) LL_TIM_IsEnabledIT_UPDATE(priv->TIMx);
    if (IT_enabled) {
        LL_TIM_DisableIT_UPDATE(priv->TIMx);
    }

    // ...
    DAC->CR &= ~(DAC_CR_EN1 | DAC_CR_EN2);

    uint32_t CR = 0;
    if (priv->cfg.ch[0].enable) {
        CR |=
            (priv->cfg.ch[0].buffered ? 0 : DAC_CR_BOFF1) |
            (priv->ch[0].noise_type << DAC_CR_WAVE1_Pos) |
            (priv->ch[0].noise_level & 0xF) << DAC_CR_MAMP1_Pos |
            DAC_CR_TEN1 |
            (0b111 << DAC_CR_TSEL1_Pos); // software trigger;

        CR |= DAC_CR_EN1;
    }

    if (priv->cfg.ch[1].enable) {
        CR |=
            (priv->cfg.ch[1].buffered ? 0 : DAC_CR_BOFF2) |
            (priv->ch[1].noise_type << DAC_CR_WAVE2_Pos) |
            (priv->ch[1].noise_level & 0xF) << DAC_CR_MAMP2_Pos |
            DAC_CR_TEN2 |
            (0b111 << DAC_CR_TSEL2_Pos); // software trigger
        CR |= DAC_CR_EN2;
    }

    DAC->CR = CR;
    // ...

    if (IT_enabled) {
        LL_TIM_EnableIT_UPDATE(priv->TIMx);
    }
}

error_t UDAC_SetFreq(Unit *unit, int channel, float freq)
{
    struct priv *priv = unit->data;

    priv->ch[channel].increment = (uint32_t) roundf(25.0f * freq * UDAC_VALUE_COUNT * // FIXME constant seems derived from the divide ...?
                                                        ((float)UDAC_TIM_FREQ_DIVIDER / PLAT_AHB_MHZ)
    );

//    dbg("Ch %d: Computed increment: %d", channel+1, (int)priv->ch[channel].increment);

    return E_SUCCESS;
}

void UDAC_ToggleTimerIfNeeded(Unit *unit)
{
    struct priv *priv = unit->data;

    if (priv->ch[0].waveform == UDAC_WAVE_DC && priv->ch[1].waveform == UDAC_WAVE_DC) {
        LL_TIM_DisableCounter(priv->TIMx);

        // manually call the interrupt function to update the level
        UDAC_HandleIT(unit);
    } else {
        LL_TIM_EnableCounter(priv->TIMx);
    }
}
