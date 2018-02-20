//
// Created by MightyPork on 2018/02/20.
//

#include "platform.h"

#define FCAP_INTERNAL
#include "_fcap_internal.h"

static void UFCAP_PWMBurstReportJob(Job *job)
{
    Unit *unit = job->unit;
    struct priv * const priv = unit->data;

    uint8_t buf[20];
    PayloadBuilder pb = pb_start(buf, 20, NULL);

    pb_u16(&pb, PLAT_AHB_MHZ);
    pb_u16(&pb, priv->pwm_burst.n_count);
    pb_u64(&pb, priv->pwm_burst.period_acu);
    pb_u64(&pb, priv->pwm_burst.ontime_acu);

    assert_param(pb.ok);

    com_respond_pb(priv->request_id, MSG_SUCCESS, &pb);

    // timer is already stopped, now in OPMODE_BUSY
    priv->opmode = OPMODE_IDLE;
}

void UFCAP_TimerHandler(void *arg)
{
    Unit *unit = arg;
    assert_param(unit);
    struct priv * const priv = unit->data;
    assert_param(priv);

    TIM_TypeDef * const TIMx = priv->TIMx;

    if (priv->opmode == OPMODE_PWM_CONT) {
        if (LL_TIM_IsActiveFlag_CC1(TIMx)) {
//            assert_param(!LL_TIM_IsActiveFlag_CC1OVR(TIMx));
            priv->pwm_cont.last_period = LL_TIM_IC_GetCaptureCH1(TIMx);
            priv->pwm_cont.last_ontime = priv->pwm_cont.ontime;
            LL_TIM_ClearFlag_CC1(TIMx);
            LL_TIM_ClearFlag_CC1OVR(TIMx);
        }

        if (LL_TIM_IsActiveFlag_CC2(TIMx)) {
//            assert_param(!LL_TIM_IsActiveFlag_CC2OVR(TIMx));
            priv->pwm_cont.ontime = LL_TIM_IC_GetCaptureCH2(TIMx);
            LL_TIM_ClearFlag_CC2(TIMx);
            LL_TIM_ClearFlag_CC2OVR(TIMx);
        }
    }
    else if (priv->opmode == OPMODE_PWM_BURST) {
        if (LL_TIM_IsActiveFlag_CC1(TIMx)) {
//            assert_param(!LL_TIM_IsActiveFlag_CC1OVR(TIMx));
            const uint32_t period = LL_TIM_IC_GetCaptureCH1(TIMx);
            const uint32_t ontime = priv->pwm_burst.ontime;

            if (priv->pwm_burst.n_skip > 0) {
                priv->pwm_burst.n_skip--;
            } else {
                priv->pwm_burst.ontime_acu += ontime;
                priv->pwm_burst.period_acu += period;
                if (++priv->pwm_burst.n_count == priv->pwm_burst.n_target) {
                    priv->opmode = OPMODE_BUSY;
                    UFCAP_StopMeasurement(unit);

                    Job j = {
                        .cb = UFCAP_PWMBurstReportJob,
                        .unit = unit,
                    };
                    scheduleJob(&j);
                }
            }

            LL_TIM_ClearFlag_CC1(TIMx);
            LL_TIM_ClearFlag_CC1OVR(TIMx);
        }

        if (LL_TIM_IsActiveFlag_CC2(TIMx)) {
//            assert_param(!LL_TIM_IsActiveFlag_CC2OVR(TIMx));
            priv->pwm_burst.ontime = LL_TIM_IC_GetCaptureCH2(TIMx);
            LL_TIM_ClearFlag_CC2(TIMx);
            LL_TIM_ClearFlag_CC2OVR(TIMx);
        }
    }
    else if (priv->opmode == OPMODE_IDLE) {
        // clear everything - in idle it would cycle in the handler forever
        TIMx->SR = 0;
    }
}

static void UFCAP_ClearTimerConfig(Unit *unit)
{
    struct priv * const priv = unit->data;
    TIM_TypeDef * const TIMx = priv->TIMx;

    // CLEAR CURRENT STATE, STOP
    UFCAP_StopMeasurement(unit);

    // CONFIGURE TIMER BASIC PARAMS
    LL_TIM_SetPrescaler(TIMx, 0);
    LL_TIM_SetAutoReload(TIMx, 0xFFFFFFFF);
    LL_TIM_EnableARRPreload(TIMx);
    LL_TIM_GenerateEvent_UPDATE(TIMx);
}

void UFCAP_StopMeasurement(Unit *unit)
{
    struct priv * const priv = unit->data;
    TIM_TypeDef * const TIMx = priv->TIMx;

    LL_TIM_DisableCounter(TIMx);
    LL_TIM_DisableIT_CC1(TIMx);
    LL_TIM_DisableIT_CC2(TIMx);
    LL_TIM_ClearFlag_CC1(TIMx);
    LL_TIM_ClearFlag_CC2(TIMx);
    LL_TIM_ClearFlag_CC1OVR(TIMx);
    LL_TIM_ClearFlag_CC2OVR(TIMx);
    LL_TIM_SetCounter(TIMx, 0);
}

void UFCAP_SwitchMode(Unit *unit, enum fcap_opmode opmode)
{
    struct priv * const priv = unit->data;

    if (opmode == priv->opmode) return;

    priv->opmode = opmode;

    switch (opmode) {
        case OPMODE_IDLE:
            // XXX maybe we should report the abort to the PC-side listener
            UFCAP_StopMeasurement(unit);
            break;

        case OPMODE_PWM_CONT:
            priv->pwm_cont.last_ontime = 0;
            priv->pwm_cont.last_period = 0;
            priv->pwm_cont.ontime = 0;
            UFCAP_ConfigureForPWMCapture(unit); // is also stopped and restarted
            break;

        case OPMODE_PWM_BURST:
            priv->pwm_burst.ontime = 0;
            priv->pwm_burst.n_count = 0;
            priv->pwm_burst.n_skip = 1; // discard the first cycle (will be incomplete)
            priv->pwm_burst.period_acu = 0;
            priv->pwm_burst.ontime_acu = 0;
            UFCAP_ConfigureForPWMCapture(unit); // is also stopped and restarted
            break;
        default:
            trap("Unhandled opmode %d", (int)opmode);
    }
}

void UFCAP_ConfigureForPWMCapture(Unit *unit)
{
    struct priv * const priv = unit->data;
    TIM_TypeDef * const TIMx = priv->TIMx;
    const uint32_t ll_ch_a = priv->ll_ch_a;
    const uint32_t ll_ch_b = priv->ll_ch_b;

    UFCAP_ClearTimerConfig(unit);

    // Enable channels and select mapping to TIx signals

    // A - will be used to measure period
    // B - will be used to measure the duty cycle

    //         _________                  ______
    // _______|         |________________|
    //        A         B                A
    //        irq       irq,cap          irq
    //        reset

    // B irq may be used if we want to measure a pulse width

    // Normally TI1 = CH1, TI2 = CH2.
    // It's possible to select the other channel, which we use to connect both TIx to the shame CHx.
    LL_TIM_IC_SetActiveInput(TIMx, ll_ch_a, priv->a_direct ? LL_TIM_ACTIVEINPUT_DIRECTTI   : LL_TIM_ACTIVEINPUT_INDIRECTTI);
    LL_TIM_IC_SetActiveInput(TIMx, ll_ch_b, priv->a_direct ? LL_TIM_ACTIVEINPUT_INDIRECTTI : LL_TIM_ACTIVEINPUT_DIRECTTI);
    LL_TIM_CC_EnableChannel(TIMx, ll_ch_a | ll_ch_b);

    LL_TIM_IC_SetPolarity(TIMx, ll_ch_a, LL_TIM_IC_POLARITY_RISING);
    LL_TIM_IC_SetPolarity(TIMx, ll_ch_b, LL_TIM_IC_POLARITY_FALLING);
    LL_TIM_SetSlaveMode(TIMx, LL_TIM_SLAVEMODE_RESET);
    LL_TIM_SetTriggerInput(TIMx, LL_TIM_TS_TI1FP1); // Use Filtered Input 1 (TI1)
    LL_TIM_EnableMasterSlaveMode(TIMx);

    LL_TIM_EnableIT_CC1(TIMx);
    LL_TIM_EnableIT_CC2(TIMx);
    LL_TIM_EnableCounter(TIMx);
}
