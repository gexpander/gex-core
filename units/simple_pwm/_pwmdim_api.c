//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_pwmdim.h"

#define PWMDIM_INTERNAL
#include "_pwmdim_internal.h"

error_t UPWMDIM_SetFreq(Unit *unit, uint32_t freq)
{
    struct priv *priv = unit->data;

    uint16_t presc;
    uint32_t count;
    float real_freq;
    if (!hw_solve_timer(PLAT_APB1_HZ, freq, true, &presc, &count, &real_freq)) {
        dbg("Failed to resolve timer params.");
        return E_BAD_VALUE;
    }
    LL_TIM_SetPrescaler(priv->TIMx, (uint32_t) (presc - 1));
    LL_TIM_SetAutoReload(priv->TIMx, count - 1);

    // we must re-calculate duty cycles because they are absolute related to the ARR which we just changed
    UPWMDIM_SetDuty(unit, 0, priv->duty1);
    UPWMDIM_SetDuty(unit, 1, priv->duty2);
    UPWMDIM_SetDuty(unit, 2, priv->duty3);
    UPWMDIM_SetDuty(unit, 3, priv->duty4);

//    LL_TIM_GenerateEvent_UPDATE(priv->TIMx); // - this appears to cause jumpiness
    priv->freq = freq;

    return E_SUCCESS;
}

error_t UPWMDIM_SetDuty(Unit *unit, uint8_t ch, uint16_t duty1000)
{
    struct priv *priv = unit->data;

    uint32_t cnt = (LL_TIM_GetAutoReload(priv->TIMx) + 1)*duty1000 / 1000;

    if (ch == 0) {
        priv->duty1 = duty1000;
        LL_TIM_OC_SetCompareCH1(priv->TIMx, cnt);
    }
    else if (ch == 1) {
        priv->duty2 = duty1000;
        LL_TIM_OC_SetCompareCH2(priv->TIMx, cnt);
    }
    else if (ch == 2) {
        priv->duty3 = duty1000;
        LL_TIM_OC_SetCompareCH3(priv->TIMx, cnt);
    }
    else if (ch == 3) {
        priv->duty4 = duty1000;
        LL_TIM_OC_SetCompareCH4(priv->TIMx, cnt);
    } else {
        return E_BAD_VALUE;
    }

    return E_SUCCESS;
}
