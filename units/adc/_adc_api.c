//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_adc.h"

#define ADC_INTERNAL
#include "_adc_internal.h"

error_t UU_ADC_AbortCapture(Unit *unit)
{
    CHECK_TYPE(unit, &UNIT_ADC);
    struct priv *priv = unit->data;

    enum uadc_opmode old_opmode = priv->opmode;

    priv->auto_rearm = false;
    UADC_SwitchMode(unit, ADC_OPMODE_IDLE);

    if (old_opmode == ADC_OPMODE_BLCAP ||
        old_opmode == ADC_OPMODE_STREAM ||
        old_opmode == ADC_OPMODE_TRIGD) {
        UADC_ReportEndOfStream(unit);
    }

    return E_SUCCESS;
}
