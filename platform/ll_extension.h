//
// Created by MightyPork on 2018/02/04.
//

#ifndef GEX_F072_LL_EXTENSION_H
#define GEX_F072_LL_EXTENSION_H

#include "platform.h"

extern const uint32_t LL_SYSCFG_EXTI_PORTS[PORTS_COUNT];
extern const uint32_t LL_SYSCFG_EXTI_LINES[16];
extern GPIO_TypeDef * const GPIO_PERIPHS[PORTS_COUNT];
extern const uint32_t LL_GPIO_PINS[16];
extern const uint32_t LL_EXTI_LINES[16];
extern const uint32_t LL_ADC_SAMPLETIMES[8];
extern const uint32_t LL_TIM_IC_FILTERS[16];
extern const uint32_t LL_TIM_ETR_FILTERS[16];

static inline bool LL_DMA_IsActiveFlag_G(uint32_t isr_snapshot, uint8_t channel)
{
    return 0 != (isr_snapshot & (DMA_ISR_GIF1 << (uint32_t)((channel-1) * 4)));
}

static inline bool LL_DMA_IsActiveFlag_TC(uint32_t isr_snapshot, uint8_t channel)
{
    return 0 != (isr_snapshot & (DMA_ISR_TCIF1 << (uint32_t)((channel-1) * 4)));
}

static inline bool LL_DMA_IsActiveFlag_HT(uint32_t isr_snapshot, uint8_t channel)
{
    return 0 != (isr_snapshot & (DMA_ISR_HTIF1 << (uint32_t)((channel-1) * 4)));
}

static inline bool LL_DMA_IsActiveFlag_TE(uint32_t isr_snapshot, uint8_t channel)
{
    return 0 != (isr_snapshot & (DMA_ISR_TEIF1 << (uint32_t)((channel-1) * 4)));
}

static inline void LL_DMA_ClearFlag_HT(DMA_TypeDef *DMAx, uint8_t channel)
{
    DMAx->IFCR = (DMA_IFCR_CHTIF1 << (uint32_t)((channel-1) * 4));
}

static inline void LL_DMA_ClearFlag_TC(DMA_TypeDef *DMAx, uint8_t channel)
{
    DMAx->IFCR = (DMA_IFCR_CTCIF1 << (uint32_t)((channel-1) * 4));
}

static inline void LL_DMA_ClearFlag_TE(DMA_TypeDef *DMAx, uint8_t channel)
{
    DMAx->IFCR = (DMA_IFCR_CTEIF1 << (uint32_t)((channel-1) * 4));
}

static inline void LL_DMA_ClearFlags(DMA_TypeDef *DMAx, uint8_t channel)
{
    DMAx->IFCR = (DMA_IFCR_CGIF1 << (uint32_t)((channel-1) * 4));
}


#endif //GEX_F072_LL_EXTENSION_H
