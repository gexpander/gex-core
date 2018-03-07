//
// Created by MightyPork on 2018/01/27.
//

#include "platform.h"
#include "timebase.h"

// ---------------------------- HAL TIMEBASE -----------------------------

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    // EDIT - used 17 instead because 14 was needed for fcap

    // TIM14 is a simple 16-bit timer timer with no special features.
    // This makes it a good choice for the timebase generation. We set it to generate
    // an interrupt every 1 ms

    assert_param(TickPriority == 0); // any other setting can lead to crashes

    // - TIM14 is always up-counting
    // - using APB1 clock
    __HAL_RCC_TIM17_CLK_ENABLE();
    NVIC_SetPriority(TIM17_IRQn, TickPriority); // highest possible priority
    NVIC_EnableIRQ(TIM17_IRQn);

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
void TIM17_IRQHandler(void)
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

        if (LL_TIM_IsActiveFlag_UPDATE(TIMEBASE_TIMER)) {
            // This means the timer has overflown after we disabled IRQ
            // Use the last CNT value before the overflow
            uwMicros = TIMEBASE_TIMER->ARR; // this is 999us
        }
    }
    vPortExitCritical();

    return (uint64_t) uwMillis * 1000 + uwMicros;
}


/** microsecond delay */
void PTIM_MicroDelayAligned(uint32_t usec, register const uint32_t start)
{
    assert_param(usec < 1000);

    register const uint32_t remain = (1000 - start);

    if (remain > usec) {
        // timer still has enough space going forward to pass the required wait time
        register const uint32_t end = start + usec;
        while (TIMEBASE_TIMER->CNT < end) {
            __NOP();
            __NOP();
            __NOP();
        }
        return;
    }
    else {
        // timer is too close to the end
        usec -= remain;
        while (1) {
            register const uint32_t t = TIMEBASE_TIMER->CNT;
            if (t < start && t >= usec) return;
        }
    }
}

/** microsecond delay */
void PTIM_MicroDelay(const uint32_t usec)
{
    PTIM_MicroDelayAligned(usec, TIMEBASE_TIMER->CNT);
}
