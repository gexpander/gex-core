//
// Created by MightyPork on 2018/02/20.
//

#include "platform.h"

#define FCAP_INTERNAL
#include "_fcap_internal.h"

static void UFCAP_StopMeasurement(Unit *unit);
static void UFCAP_ConfigureForIndirectCapture(Unit *unit);
static void UFCAP_ConfigureForDirectCapture(Unit *unit, uint16_t msec);
static void UFCAP_ConfigureForFreeCapture(Unit *unit);

uint32_t UFCAP_GetFreeCounterValue(Unit *unit)
{
    struct priv * const priv = unit->data;
    TIM_TypeDef * const TIMx = priv->TIMx;
    return TIMx->CNT;
}

uint32_t UFCAP_FreeCounterClear(Unit *unit)
{
    struct priv * const priv = unit->data;
    TIM_TypeDef * const TIMx = priv->TIMx;

    // this isn't perfect, we can miss one clock
    // but it's probably the best we can do here ...
    vPortEnterCritical();
    uint32_t val = TIMx->CNT;
    TIMx->CNT = 0;
    vPortExitCritical();

    return val;
}

static void UFCAP_IndirectBurstReportJob(Job *job)
{
    Unit *unit = job->unit;
    struct priv * const priv = unit->data;

    uint8_t buf[20];
    PayloadBuilder pb = pb_start(buf, 20, NULL);

    pb_u16(&pb, PLAT_AHB_MHZ);
    pb_u16(&pb, priv->ind_burst.n_count);
    pb_u64(&pb, priv->ind_burst.period_acu);
    pb_u64(&pb, priv->ind_burst.ontime_acu);

    assert_param(pb.ok);

    com_respond_pb(priv->request_id, MSG_SUCCESS, &pb);

    // timer is already stopped, now in OPMODE_BUSY
    priv->opmode = OPMODE_IDLE;
}

static void UFCAP_SinglePulseReportJob(Job *job)
{
    Unit *unit = job->unit;
    struct priv * const priv = unit->data;

    uint8_t buf[6];
    PayloadBuilder pb = pb_start(buf, 6, NULL);

    pb_u16(&pb, PLAT_AHB_MHZ);
    pb_u32(&pb, job->data1);
    assert_param(pb.ok);

    com_respond_pb(priv->request_id, MSG_SUCCESS, &pb);

    // timer is already stopped, now in OPMODE_BUSY
    priv->opmode = OPMODE_IDLE;
}

/**
 * Count is passed in data1
 * @param job
 */
static void UFCAP_DirectBurstReportJob(Job *job)
{
    Unit *unit = job->unit;
    struct priv * const priv = unit->data;

    uint8_t buf[8];
    PayloadBuilder pb = pb_start(buf, 8, NULL);

    pb_u8(&pb, priv->direct_presc);
    pb_u16(&pb, priv->dir_burst.msec);
    pb_u32(&pb, job->data1);

    assert_param(pb.ok);

    com_respond_pb(priv->request_id, MSG_SUCCESS, &pb);

    // timer is already stopped, now in OPMODE_BUSY
    priv->opmode = OPMODE_IDLE;
}

void UFCAP_TIMxHandler(void *arg)
{
    Unit *unit = arg;
    assert_param(unit);
    struct priv * const priv = unit->data;
    assert_param(priv);

    TIM_TypeDef * const TIMx = priv->TIMx;

    if (priv->opmode == OPMODE_INDIRECT_CONT) {
        if (LL_TIM_IsActiveFlag_CC1(TIMx)) {
            if (priv->n_skip > 0) {
                priv->n_skip--;
            } else {
                priv->ind_cont.last_period = LL_TIM_IC_GetCaptureCH1(TIMx);
                priv->ind_cont.last_ontime = priv->ind_cont.ontime;
            }
            LL_TIM_ClearFlag_CC1(TIMx);
            LL_TIM_ClearFlag_CC1OVR(TIMx);
        }

        if (LL_TIM_IsActiveFlag_CC2(TIMx)) {
            priv->ind_cont.ontime = LL_TIM_IC_GetCaptureCH2(TIMx);
            LL_TIM_ClearFlag_CC2(TIMx);
            LL_TIM_ClearFlag_CC2OVR(TIMx);
        }
    }
    else if (priv->opmode == OPMODE_SINGLE_PULSE) {
        if (LL_TIM_IsActiveFlag_CC2(TIMx)) {
            // single pulse - does not wait for the second edge
            uint32_t len = LL_TIM_IC_GetCaptureCH2(TIMx);

            priv->opmode = OPMODE_BUSY;
            UFCAP_StopMeasurement(unit);

            Job j = {
                .cb = UFCAP_SinglePulseReportJob,
                .unit = unit,
                .data1 = len,
            };
            scheduleJob(&j);
        }
    }
    else if (priv->opmode == OPMODE_INDIRECT_BURST) {
        if (LL_TIM_IsActiveFlag_CC1(TIMx)) {
            const uint32_t period = LL_TIM_IC_GetCaptureCH1(TIMx);
            const uint32_t ontime = priv->ind_burst.ontime;

            if (priv->n_skip > 0) {
                priv->n_skip--;
            } else {
                priv->ind_burst.ontime_acu += ontime;
                priv->ind_burst.period_acu += period;
                if (++priv->ind_burst.n_count == priv->ind_burst.n_target) {
                    priv->opmode = OPMODE_BUSY;
                    UFCAP_StopMeasurement(unit);

                    Job j = {
                        .cb = UFCAP_IndirectBurstReportJob,
                        .unit = unit,
                    };
                    scheduleJob(&j);
                }
            }

            LL_TIM_ClearFlag_CC1(TIMx);
            LL_TIM_ClearFlag_CC1OVR(TIMx);
        }

        if (LL_TIM_IsActiveFlag_CC2(TIMx)) {
            priv->ind_burst.ontime = LL_TIM_IC_GetCaptureCH2(TIMx);
            LL_TIM_ClearFlag_CC2(TIMx);
            LL_TIM_ClearFlag_CC2OVR(TIMx);
        }
    }
    else if (priv->opmode == OPMODE_IDLE) {
        // clear everything - in idle it would cycle in the handler forever
        TIMx->SR = 0;
    }
    else {
        trap("Unhandled fcap TIMx irq");
    }
}

void UFCAP_TIMyHandler(void *arg)
{
    Unit *unit = arg;
    assert_param(unit);
    struct priv *const priv = unit->data;
    assert_param(priv);

    TIM_TypeDef * const TIMx = priv->TIMx;
    TIM_TypeDef * const TIMy = priv->TIMy;
    uint32_t cnt = TIMx->CNT; // TIMx should be stopped now

//    dbg("> TIMy Handler, TIMx cntr is %d", cnt);
    priv->dir_cont.last_count = cnt;

    if (priv->opmode == OPMODE_DIRECT_CONT) {
        LL_TIM_DisableCounter(TIMx);
        LL_TIM_DisableCounter(TIMy);
        LL_TIM_SetCounter(TIMx, 0);
        LL_TIM_SetCounter(TIMy, 0);
        LL_TIM_EnableCounter(TIMy); // next loop
        LL_TIM_EnableCounter(TIMx);
    }
    else if (priv->opmode == OPMODE_DIRECT_BURST) {
        priv->opmode = OPMODE_BUSY;
        UFCAP_StopMeasurement(unit);

        Job j = {
            .cb = UFCAP_DirectBurstReportJob,
            .unit = unit,
            .data1 = cnt,
        };
        scheduleJob(&j);
    }
    else if (priv->opmode == OPMODE_IDLE) {
        // clear everything - in idle it would cycle in the handler forever
        TIMy->SR = 0;
    }
    else {
        trap("Unhandled fcap TIMy irq");
    }

    LL_TIM_ClearFlag_UPDATE(TIMy);
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

/**
 * Reset all timer registers
 *
 * @param unit
 */
static void UFCAP_StopMeasurement(Unit *unit)
{
    struct priv * const priv = unit->data;

    LL_TIM_DeInit(priv->TIMx); // clear all flags and settings
    LL_TIM_DeInit(priv->TIMy); // clear all flags and settings
}

/**
 * Switch the FCAP module opmode
 *
 * @param unit
 * @param opmode
 */
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

        case OPMODE_INDIRECT_CONT:
            priv->ind_cont.last_ontime = 0;
            priv->ind_cont.last_period = 0;
            priv->ind_cont.ontime = 0;
            priv->n_skip = 1; // discard the first cycle (will be incomplete)
            UFCAP_ConfigureForIndirectCapture(unit); // is also stopped and restarted
            break;

        case OPMODE_INDIRECT_BURST:
            priv->ind_burst.ontime = 0;
            priv->ind_burst.n_count = 0;
            priv->ind_burst.period_acu = 0;
            priv->ind_burst.ontime_acu = 0;
            priv->n_skip = 1; // discard the first cycle (will be incomplete)
            UFCAP_ConfigureForIndirectCapture(unit); // is also stopped and restarted
            break;

        case OPMODE_SINGLE_PULSE:
            priv->n_skip = 0;
            UFCAP_ConfigureForIndirectCapture(unit); // is also stopped and restarted
            break;

        case OPMODE_DIRECT_CONT:
            // msec is set by caller
            priv->dir_cont.last_count = 0;
            priv->n_skip = 1; // discard the first cycle (will be incomplete)
            UFCAP_ConfigureForDirectCapture(unit, priv->direct_msec);
            break;

        case OPMODE_DIRECT_BURST:
            // msec is set by caller
            priv->n_skip = 0; // no skip here (if there was any)
            UFCAP_ConfigureForDirectCapture(unit, (uint16_t) priv->dir_burst.msec);
            break;

        case OPMODE_FREE_COUNTER:
            UFCAP_ConfigureForFreeCapture(unit);
            break;

        default:
            trap("Unhandled opmode %d", (int)opmode);
    }
}

/**
 * Configure peripherals for an indirect capture (PWM measurement) - continuous or burst
 * @param unit
 */
static void UFCAP_ConfigureForIndirectCapture(Unit *unit)
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
    LL_TIM_IC_SetPolarity(TIMx, ll_ch_a, priv->active_level ? LL_TIM_IC_POLARITY_RISING : LL_TIM_IC_POLARITY_FALLING);
    LL_TIM_IC_SetPolarity(TIMx, ll_ch_b, priv->active_level ? LL_TIM_IC_POLARITY_FALLING : LL_TIM_IC_POLARITY_RISING);

    if (priv->dfilter > 15) priv->dfilter = 15;
    uint32_t filter = LL_TIM_IC_FILTERS[priv->dfilter];
    LL_TIM_IC_SetFilter(TIMx, ll_ch_a, filter);
    LL_TIM_IC_SetFilter(TIMx, ll_ch_b, filter);

    LL_TIM_CC_EnableChannel(TIMx, ll_ch_a | ll_ch_b);

    LL_TIM_SetSlaveMode(TIMx, LL_TIM_SLAVEMODE_RESET);
    LL_TIM_SetTriggerInput(TIMx, LL_TIM_TS_TI1FP1); // Use Filtered Input 1 (TI1)

    LL_TIM_EnableMasterSlaveMode(TIMx);

    LL_TIM_ClearFlag_CC1(TIMx);
    LL_TIM_ClearFlag_CC1OVR(TIMx);
    LL_TIM_ClearFlag_CC2(TIMx);
    LL_TIM_ClearFlag_CC2OVR(TIMx);

    LL_TIM_EnableIT_CC1(TIMx);
    LL_TIM_EnableIT_CC2(TIMx);
    LL_TIM_EnableCounter(TIMx);
}


/**
 * Configure peripherals for an indirect capture (PWM measurement) - continuous or burst
 * @param unit
 */
static void UFCAP_ConfigureForDirectCapture(Unit *unit, uint16_t msec)
{
    struct priv * const priv = unit->data;

//    dbg("Configuring Direct capture...");

    UFCAP_ClearTimerConfig(unit);

    {
        TIM_TypeDef *const TIMy = priv->TIMy;
        assert_param(PLAT_AHB_MHZ<=65);
        uint16_t presc = PLAT_AHB_MHZ*1000;
        uint32_t count = msec+1; // it's one tick longer because we generate OCREF on the exact msec count - it must be at least 1 tick long

        LL_TIM_SetPrescaler(TIMy, (uint32_t) (presc - 1));
        LL_TIM_SetAutoReload(TIMy, count - 1);
        LL_TIM_EnableARRPreload(TIMy);
        LL_TIM_GenerateEvent_UPDATE(TIMy);
        LL_TIM_SetOnePulseMode(TIMy, LL_TIM_ONEPULSEMODE_SINGLE);
        LL_TIM_OC_EnableFast(TIMy, LL_TIM_CHANNEL_CH1);

//        dbg("TIMy presc %d, count %d", (int) presc, (int) count);

        LL_TIM_SetTriggerOutput(TIMy, LL_TIM_TRGO_OC1REF);
        LL_TIM_OC_SetMode(TIMy, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1); // 1 until CC, then 0
        LL_TIM_OC_SetCompareCH1(TIMy, count-1);
        LL_TIM_CC_EnableChannel(TIMy, LL_TIM_CHANNEL_CH1); // enable the output channel that produces a trigger

        LL_TIM_ClearFlag_UPDATE(TIMy);
        LL_TIM_EnableIT_UPDATE(TIMy);
    }

    {
        // TIMx - the slave
        TIM_TypeDef *const TIMx = priv->TIMx;
        LL_TIM_SetSlaveMode(TIMx, LL_TIM_SLAVEMODE_GATED);
        LL_TIM_SetTriggerInput(TIMx, LL_TIM_TS_ITR3); // ITR3 is TIM14 which we use as TIMy
        LL_TIM_EnableMasterSlaveMode(TIMx);

        uint32_t presc = LL_TIM_ETR_PRESCALER_DIV1;
        switch (priv->direct_presc) {
            case 1: presc = LL_TIM_ETR_PRESCALER_DIV1; break;
            case 2: presc = LL_TIM_ETR_PRESCALER_DIV2; break;
            case 4: presc = LL_TIM_ETR_PRESCALER_DIV4; break;
            case 8: presc = LL_TIM_ETR_PRESCALER_DIV8; break;
            default:
                priv->direct_presc = 1; // will be sent with the response
        }

        if (priv->dfilter > 15) priv->dfilter = 15;
        uint32_t filter = LL_TIM_ETR_FILTERS[priv->dfilter];

        LL_TIM_ConfigETR(TIMx,
                         priv->active_level ? LL_TIM_ETR_POLARITY_NONINVERTED : LL_TIM_ETR_POLARITY_INVERTED,
                         presc,
                         filter);

        LL_TIM_EnableExternalClock(TIMx); // TODO must check and deny this mode if the pin is not on CH1 = external trigger input

        LL_TIM_SetCounter(TIMx, 0);
        LL_TIM_EnableCounter(TIMx);
    }

    LL_TIM_EnableCounter(priv->TIMy); // XXX this will start the first pulse (maybe)
}


/**
 * Freerunning capture (counting pulses - geiger)
 * @param unit
 */
static void UFCAP_ConfigureForFreeCapture(Unit *unit)
{
    struct priv * const priv = unit->data;

    UFCAP_ClearTimerConfig(unit);

    TIM_TypeDef *const TIMx = priv->TIMx;
    LL_TIM_EnableExternalClock(TIMx);
    LL_TIM_SetCounter(TIMx, 0);
    LL_TIM_EnableCounter(TIMx);
}
