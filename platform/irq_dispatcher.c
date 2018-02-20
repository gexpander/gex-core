//
// Created by MightyPork on 2018/01/14.
//

#include "platform.h"
#include "irq_dispatcher.h"
#include "hw_utils.h"

void * const EXTIS[16] = {
    // dumym values for easier debug - those are used instead of periph addresses because EXTIs have no dedicated periph control blocks
    (void *const) 0x9000,
    (void *const) 0x9001,
    (void *const) 0x9002,
    (void *const) 0x9003,
    (void *const) 0x9004,
    (void *const) 0x9005,
    (void *const) 0x9006,
    (void *const) 0x9007,
    (void *const) 0x9008,
    (void *const) 0x9009,
    (void *const) 0x900A,
    (void *const) 0x900B,
    (void *const) 0x900C,
    (void *const) 0x900D,
    (void *const) 0x900E,
    (void *const) 0x900F,
};

struct cbslot {
    IrqCallback callback;
    void *arg;
};

static struct callbacks_ {
    struct cbslot exti[16];

    struct cbslot usart1;
    struct cbslot usart2;
    struct cbslot usart3;
#ifdef USART4
    struct cbslot usart4;
#endif
#ifdef USART5
    struct cbslot usart5;
#endif
    struct cbslot dma1_1;
    struct cbslot dma1_2;
    struct cbslot dma1_3;
    struct cbslot dma1_4;
    struct cbslot dma1_5;
    struct cbslot dma1_6;
    struct cbslot dma1_7;
    struct cbslot dma1_8;

    struct cbslot dma2_1;
    struct cbslot dma2_2;
    struct cbslot dma2_3;
    struct cbslot dma2_4;
    struct cbslot dma2_5;
    struct cbslot dma2_6;
    struct cbslot dma2_7;
    struct cbslot dma2_8;

    struct cbslot tim2;
    struct cbslot tim6;
    struct cbslot tim7;
    struct cbslot tim14;
    struct cbslot tim15;
    struct cbslot tim16;

    struct cbslot adc1;

    // XXX add more callbacks here when needed
} callbacks;

void irqd_init(void)
{
    memset(&callbacks, 0, sizeof(callbacks));

    // TODO move the priorities to some define

//    NVIC_EnableIRQ(WWDG_IRQn);                  /*!< Window WatchDog Interrupt                                       */
//    NVIC_EnableIRQ(PVD_VDDIO2_IRQn);            /*!< PVD & VDDIO2 Interrupt through EXTI Lines 16 and 31             */
//    NVIC_EnableIRQ(RTC_IRQn);                   /*!< RTC Interrupt through EXTI Lines 17, 19 and 20                  */
//    NVIC_EnableIRQ(FLASH_IRQn);                 /*!< FLASH global Interrupt                                          */
//    NVIC_EnableIRQ(RCC_CRS_IRQn);               /*!< RCC & CRS global Interrupt                                      */

    NVIC_EnableIRQ(EXTI0_1_IRQn);               /*!< EXTI Line 0 and 1 Interrupt                                     */
    NVIC_EnableIRQ(EXTI2_3_IRQn);               /*!< EXTI Line 2 and 3 Interrupt                                     */
    NVIC_EnableIRQ(EXTI4_15_IRQn);              /*!< EXTI Line 4 to 15 Interrupt                                     */
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 2, 0);
    HAL_NVIC_SetPriority(EXTI2_3_IRQn, 2, 0);
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 2, 0);

//    NVIC_EnableIRQ(TSC_IRQn);                   /*!< Touch Sensing Controller Interrupts                             */

    NVIC_EnableIRQ(DMA1_Channel1_IRQn);         /*!< DMA1 Channel 1 Interrupt                                        */
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);       /*!< DMA1 Channel 2 and Channel 3 Interrupt                          */
    NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);   /*!< DMA1 Channel 4 to Channel 7 Interrupt                           */
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 2, 0);
    HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 2, 0);
    HAL_NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 2, 0);

    NVIC_EnableIRQ(ADC1_COMP_IRQn);             /*!< ADC1 and COMP interrupts (ADC interrupt combined with EXTI Lines 21 and 22 */
    HAL_NVIC_SetPriority(ADC1_COMP_IRQn, 1, 0); // ADC group completion - higher prio than DMA to let it handle the last halfword first

//    NVIC_EnableIRQ(TIM1_IRQn);                  /*!< TIM1 global Interrupt                                          */
    NVIC_EnableIRQ(TIM2_IRQn);                  /*!< TIM2 global Interrupt                                           */
    HAL_NVIC_SetPriority(TIM2_IRQn, 2, 0); // Used by FCAP

//    NVIC_EnableIRQ(TIM3_IRQn);                  /*!< TIM3 global Interrupt                                           */

    NVIC_EnableIRQ(TIM6_DAC_IRQn);              /*!< TIM6 global and DAC channel underrun error Interrupt            */
    HAL_NVIC_SetPriority(TIM7_IRQn, 2, 0); // Used for DAC timing

    NVIC_EnableIRQ(TIM7_IRQn);                  /*!< TIM7 global Interrupt                                           */
    HAL_NVIC_SetPriority(TIM7_IRQn, 2, 0);// this will be for dac (?)

    NVIC_EnableIRQ(TIM14_IRQn); /*used by fcap as a time reference for direct capture */ /*!< TIM14 global Interrupt */
    HAL_NVIC_SetPriority(TIM14_IRQn, 2, 0);

    NVIC_EnableIRQ(TIM15_IRQn);                 /*!< TIM15 global Interrupt                                          */
    HAL_NVIC_SetPriority(TIM15_IRQn, 2, 0); // Used by ADC

    NVIC_EnableIRQ(TIM16_IRQn);                  /*!< TIM16 global Interrupt                                          */
    HAL_NVIC_SetPriority(TIM16_IRQn, 2, 0);


    /* Tim17 is used for HAL timebase, because SysTick is used to time FreeRTOS and has the lowest priority. */
    /* Tim17's priority is set to 0 in the init routine, which runs early in the startup sequence */
//    NVIC_EnableIRQ(TIM17_IRQn);                 /*!< TIM17 global Interrupt                                          */


//    NVIC_EnableIRQ(I2C1_IRQn);                  /*!< I2C1 Event Interrupt & EXTI Line23 Interrupt (I2C1 wakeup)      */
//    NVIC_EnableIRQ(I2C2_IRQn);                  /*!< I2C2 Event Interrupt                                            */
//    NVIC_EnableIRQ(SPI1_IRQn);                  /*!< SPI1 global Interrupt                                           */
//    NVIC_EnableIRQ(SPI2_IRQn);                  /*!< SPI2 global Interrupt                                           */

    NVIC_EnableIRQ(USART1_IRQn);                /*!< USART1 global Interrupt & EXTI Line25 Interrupt (USART1 wakeup) */
    NVIC_EnableIRQ(USART2_IRQn);                /*!< USART2 global Interrupt & EXTI Line26 Interrupt (USART2 wakeup) */
    NVIC_EnableIRQ(USART3_4_IRQn);              /*!< USART3 and USART4 global Interrupt                              */
    HAL_NVIC_SetPriority(USART1_IRQn, 2, 0);
    HAL_NVIC_SetPriority(USART2_IRQn, 2, 0);
    HAL_NVIC_SetPriority(USART3_4_IRQn, 2, 0);

//    NVIC_EnableIRQ(CEC_CAN_IRQn);               /*!< CEC and CAN global Interrupts & EXTI Line27 Interrupt           */

//    NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);   /*!< TIM1 Break, Update, Trigger and Commutation Interrupt           */ // - handled by hal msp init
//    NVIC_EnableIRQ(TIM1_CC_IRQn);               /*!< TIM1 Capture Compare Interrupt                                  */
//    NVIC_EnableIRQ(USB_IRQn);                   /*!< USB global Interrupt  & EXTI Line18 Interrupt                   */ // - USB IRQ is handled by the USB library
}

//void Default_Handler(void)
//{
//    warn("Missing IRQHandler, ISPR[0]=0x%p", (void*)NVIC->ISPR[0]);
//}


static struct cbslot *get_slot_for_periph(void *periph)
{
    struct cbslot *slot = NULL;
    // --- USART ---
    if (periph == USART1) slot = &callbacks.usart1;
    else if (periph == USART2) slot = &callbacks.usart2;
    else if (periph == USART3) slot = &callbacks.usart3;
#ifdef USART4
    else if (periph == USART4) slot = &callbacks.usart4;
#endif
#ifdef USART5
        else if (periph == USART5) slot = &callbacks.usart5;
#endif

    else if (periph == TIM2) slot = &callbacks.tim2;
    else if (periph == TIM6) slot = &callbacks.tim6;
    else if (periph == TIM7) slot = &callbacks.tim7;
    else if (periph == TIM14) slot = &callbacks.tim14;
    else if (periph == TIM15) slot = &callbacks.tim15;
    else if (periph == TIM16) slot = &callbacks.tim16;
    // 17 - used by timebase

    else if (periph == ADC1) slot = &callbacks.adc1;

    else if (periph >= EXTIS[0] && periph <= EXTIS[15]) {
        slot = &callbacks.exti[periph - EXTIS[0]];
    }

        // --- DMA1 ---
    else if (periph == DMA1_Channel1) slot = &callbacks.dma1_1;
    else if (periph == DMA1_Channel2) slot = &callbacks.dma1_2;
    else if (periph == DMA1_Channel3) slot = &callbacks.dma1_3;
    else if (periph == DMA1_Channel4) slot = &callbacks.dma1_4;
    else if (periph == DMA1_Channel5) slot = &callbacks.dma1_5;
    else if (periph == DMA1_Channel6) slot = &callbacks.dma1_6;
    else if (periph == DMA1_Channel7) slot = &callbacks.dma1_7;
#ifdef DMA1_Channel8
        else if (periph == DMA1_Channel7) slot = &callbacks.dma1_8;
#endif

        // --- DMA2 ---
#ifdef DMA2
        else if (periph == DMA2_Channel1) slot = &callbacks.dma2_1;
    else if (periph == DMA2_Channel2) slot = &callbacks.dma2_2;
    else if (periph == DMA2_Channel3) slot = &callbacks.dma2_3;
    else if (periph == DMA2_Channel4) slot = &callbacks.dma2_4;
    else if (periph == DMA2_Channel5) slot = &callbacks.dma2_5;
    #ifdef DMA2_Channel6
        else if (periph == DMA2_Channel6) slot = &callbacks.dma2_6;
    #endif
    #ifdef DMA2_Channel7
        else if (periph == DMA2_Channel7) slot = &callbacks.dma2_7;
    #endif
    #ifdef DMA2_Channel8
        else if (periph == DMA2_Channel7) slot = &callbacks.dma2_8;
    #endif
#endif
    else {
        trap("No IRQ cb slot for periph 0x%p", periph);
    }

    return slot;
}

void irqd_attach(void *periph, IrqCallback callback, void *arg)
{
    struct cbslot *slot = get_slot_for_periph(periph);
    if (slot->callback != NULL) {
        trap("Attach IRQ %p() to %p but %p() already bound", callback, periph, slot->callback);
    }

    slot->callback = callback;
    slot->arg = arg;
}

void* irqd_detach(void *periph, IrqCallback callback)
{
    void *oldArg = NULL;
    struct cbslot *slot = get_slot_for_periph(periph);
    if (slot->callback == callback) {
        oldArg = slot->arg;
        slot->callback = NULL;
        slot->arg = NULL;

        return oldArg;
    } else {
        trap("Detach IRQ %p() from %p but %p() bound instead", callback, periph, slot->callback);
    }
}

#define CALL_IRQ_HANDLER(slot) do { if (slot.callback) slot.callback(slot.arg); } while (0)

// ------------ USARTS ------------

void USART1_IRQHandler(void)
{
    CALL_IRQ_HANDLER(callbacks.usart1);
}

void USART2_IRQHandler(void)
{
    CALL_IRQ_HANDLER(callbacks.usart2);
}

void USART3_4_IRQHandler(void)
{
    // we won't check the flags here, both can be pending and it's better to let the handler deal with it
    CALL_IRQ_HANDLER(callbacks.usart3);
    CALL_IRQ_HANDLER(callbacks.usart4);
}

// ------------ DMA --------------

void DMA1_Channel1_IRQHandler(void)
{
    CALL_IRQ_HANDLER(callbacks.dma1_1);
}

void DMA1_Channel2_3_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_GI2(DMA1)) CALL_IRQ_HANDLER(callbacks.dma1_2);
    if (LL_DMA_IsActiveFlag_GI3(DMA1)) CALL_IRQ_HANDLER(callbacks.dma1_3);
}

void DMA1_Channel4_5_6_7_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_GI4(DMA1)) CALL_IRQ_HANDLER(callbacks.dma1_4);
    if (LL_DMA_IsActiveFlag_GI5(DMA1)) CALL_IRQ_HANDLER(callbacks.dma1_5);
    if (LL_DMA_IsActiveFlag_GI6(DMA1)) CALL_IRQ_HANDLER(callbacks.dma1_6);
    if (LL_DMA_IsActiveFlag_GI7(DMA1)) CALL_IRQ_HANDLER(callbacks.dma1_7);
}

// ------------ EXTI ------------

static void fire_exti_cb(uint8_t start, uint8_t end)
{
    for (int i = start; i <= end; i++) {
        if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINES[i])) {
            CALL_IRQ_HANDLER(callbacks.exti[i]);
        }
    }
}

void EXTI0_1_IRQHandler(void)
{
    fire_exti_cb(0, 1);
}

void EXTI2_3_IRQHandler(void)
{
    fire_exti_cb(2, 3);
}

void EXTI4_15_IRQHandler(void)
{
    fire_exti_cb(4, 15);
}


// ------------ INTERRUPTS -------------

// TIM14 is used to generate HAL timebase and its handler is in the file "timebase.c"

void TIM2_IRQHandler(void)
{
    CALL_IRQ_HANDLER(callbacks.tim2);
}

void TIM6_DAC_IRQHandler(void)
{
    CALL_IRQ_HANDLER(callbacks.tim6);
}

void TIM7_IRQHandler(void)
{
    CALL_IRQ_HANDLER(callbacks.tim7);
}

void TIM15_IRQHandler(void)
{
    CALL_IRQ_HANDLER(callbacks.tim15);
}

void TIM16_IRQHandler(void)
{
    CALL_IRQ_HANDLER(callbacks.tim16);
}

void ADC1_COMP_IRQHandler(void)
{
    CALL_IRQ_HANDLER(callbacks.adc1);
}


// other ISRs...

#if 0
void WWDG_IRQHandler(void)
{
    //
}

void PVD_VDDIO2_IRQHandler(void)
{
    //
}

void RTC_IRQHandler(void)
{
    //
}

void FLASH_IRQHandler(void)
{
    //
}

void RCC_CRS_IRQHandler(void)
{
    //
}

void TSC_IRQHandler(void)
{
    //
}

void ADC1_COMP_IRQHandler(void)
{
    //
}

void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
    //
}

void TIM1_CC_IRQHandler(void)
{
    //
}

void TIM2_IRQHandler(void)
{
    //
}

void TIM3_IRQHandler(void)
{
    //
}

// not on F072
void TIM4_IRQHandler(void)
{
    //
}

// not on F072
void TIM5_IRQHandler(void)
{
    //
}

void TIM6_DAC_IRQHandler(void)
{
    //
}

void TIM7_IRQHandler(void)
{
    //
}

void TIM8_IRQHandler(void)
{
    //
}

void TIM9_IRQHandler(void)
{
    //
}

void TIM10_IRQHandler(void)
{
    //
}

void TIM11_IRQHandler(void)
{
    //
}

void TIM12_IRQHandler(void)
{
    //
}

void TIM13_IRQHandler(void)
{
    //
}

void TIM14_IRQHandler(void)
{
    //
}

void TIM15_IRQHandler(void)
{
    //
}

void TIM16_IRQHandler(void)
{
    //
}

void TIM17_IRQHandler(void)
{
    //
}

void CEC_CAN_IRQHandler(void)
{
    //
}

// USB is handled by the USB driver

//void USB_IRQHandler(void)
//{
//    //
//}
#endif
