//
// Created by MightyPork on 2018/01/27.
//

#include "platform.h"
#include "timebase.h"

// ---------------------------- HAL TIMEBASE -----------------------------

#define TIMEBASE_TIMER TIM14

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    // TIM14 is a simple 16-bit timer timer with no special features.
    // This makes it a good choice for the timebase generation. We set it to generate
    // an interrupt every 1 ms

    assert_param(TickPriority == 0); // any other setting can lead to crashes

    // - TIM14 is always up-counting
    // - using APB1 clock
    __HAL_RCC_TIM14_CLK_ENABLE();
    NVIC_SetPriority(TIM14_IRQn, TickPriority); // highest possible priority
    NVIC_EnableIRQ(TIM14_IRQn);

    /* Compute TIM1 clock */
    uint32_t uwTimclock = HAL_RCC_GetPCLK1Freq();
    /* Get a 1 MHz clock for the timer */
    uint16_t uhPrescalerValue = (uint16_t) ((uwTimclock / 1000000) - 1);
    /* Get 1 kHz interrupt */
    uint16_t uhPeriod = (1000000 / 1000) - 1;

    LL_TIM_SetPrescaler(TIMEBASE_TIMER, uhPrescalerValue);
    LL_TIM_SetAutoReload(TIMEBASE_TIMER, uhPeriod);
    LL_TIM_EnableARRPreload(TIMEBASE_TIMER);
    LL_TIM_EnableIT_UPDATE(TIMEBASE_TIMER);
    LL_TIM_GenerateEvent_UPDATE(TIMEBASE_TIMER);

    LL_TIM_EnableCounter(TIMEBASE_TIMER);

    /* Return function status */
    return HAL_OK;
}

static volatile uint32_t uwUptimeMs = 0;

/* TIMEBASE TIMER ISR */
void TIM14_IRQHandler(void)
{
    uwUptimeMs++;
    LL_TIM_ClearFlag_UPDATE(TIMEBASE_TIMER);
}

void HAL_IncTick(void)
{
    uwUptimeMs++;
}

uint32_t HAL_GetTick(void)
{
    return uwUptimeMs;
}

void HAL_SuspendTick(void)
{
    LL_TIM_DisableIT_UPDATE(TIMEBASE_TIMER);
}

void HAL_ResumeTick(void)
{
    LL_TIM_EnableIT_UPDATE(TIMEBASE_TIMER);
}


// -------------------------- DELAY FUNCTIONS AND TS -----------------------------

/*
 * I wanted to freeze time. I wanted to savor that moment, to live in that moment
 * for a week. But I couldn't stop it, only slow it. And before I knew it, she was gone.
 * -- Ben Willis, "Cashback"
 */
uint64_t PTIM_GetMicrotime(void)
{
    uint32_t uwMicros;
    uint32_t uwMillis;

    vPortEnterCritical();
    {
        uwMicros = TIMEBASE_TIMER->CNT;
        uwMillis = uwUptimeMs;

        if (LL_TIM_IsActiveFlag_UPDATE(TIM14)) {
            // This means the timer has overflown after we disabled IRQ
            // Use the last CNT value before the overflow
            uwMicros = TIM14->ARR; // this is 999us
        }
    }
    vPortExitCritical();

    return (uint64_t) uwMillis * 1000 + uwMicros;
}

/** microsecond delay */
void PTIM_MicroDelay(uint16_t usec)
{
    assert_param(usec < 1000);

    const uint16_t start = (uint16_t) TIMEBASE_TIMER->CNT;
    const uint16_t remain = (uint16_t) (999 - start);

    if (remain > usec) {
        // timer still has enough space going forward to pass the required wait time
        for (; TIMEBASE_TIMER->CNT < start + usec;);
    }
    else {
        // timer is too close to the end
        usec -= remain;
        for (; TIMEBASE_TIMER->CNT >= start || TIMEBASE_TIMER->CNT < usec;);
    }
}
