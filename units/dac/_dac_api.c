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

    DAC->CR &= ~(DAC_CR_EN1 | DAC_CR_EN2);
    uint32_t CR = 0;
    if (priv->cfg.ch1.enable) {
        CR |=
            (priv->cfg.ch1.buffered ? 0 : DAC_CR_BOFF1) |
            (priv->cfg.ch1.noise_type << DAC_CR_WAVE1_Pos) |
            (priv->cfg.ch1.noise_level & 0xF) << DAC_CR_MAMP1_Pos |
            (0b111 << DAC_CR_TSEL1_Pos); // software trigger;

        CR |= DAC_CR_EN1;
    }
    if (priv->cfg.ch2.enable) {
        CR |=
            (priv->cfg.ch2.buffered ? 0 : DAC_CR_BOFF2) |
            (priv->cfg.ch2.noise_type << DAC_CR_WAVE2_Pos) |
            (priv->cfg.ch2.noise_level & 0xF) << DAC_CR_MAMP2_Pos |
            (0b111 << DAC_CR_TSEL2_Pos); // software trigger
        CR |= DAC_CR_EN2;
    }
    DAC->CR = CR;
}
