//
// Created by MightyPork on 2018/01/06.
//
// Enum of all defined resources
//

#ifndef GEX_CORE_RSC_ENUM_H
#define GEX_CORE_RSC_ENUM_H

// X macro: Resource name,
#define XX_RESOURCES \
    X(SPI1) X(SPI2) X(SPI3) \
    X(I2C1) X(I2C2) X(I2C3) \
    X(ADC1) X(ADC2) X(ADC3) X(ADC4) \
    X(DAC1) \
    X(TSC) \
    X(USART1) X(USART2) X(USART3) X(USART4) X(USART5) X(USART6) \
    X(TIM1) X(TIM2) X(TIM3) X(TIM4) X(TIM5) \
    X(TIM6) X(TIM7) X(TIM8) X(TIM9) X(TIM10) X(TIM11) X(TIM12) X(TIM13) X(TIM14) \
    X(TIM15) X(TIM16) X(TIM17) \
    X(DMA1_1) X(DMA1_2) X(DMA1_3) X(DMA1_4) X(DMA1_5) X(DMA1_6) X(DMA1_7) X(DMA1_8) \
    X(DMA2_1) X(DMA2_2) X(DMA2_3) X(DMA2_4) X(DMA2_5) X(DMA2_6) X(DMA2_7) X(DMA2_8)

// Resources not used anywhere:
// X(I2S1) X(I2S2) X(I2S3)
// X(OPAMP1) X(OPAMP2) X(OPAMP3) X(OPAMP4)
// X(CAN1) X(CAN2)
// X(DCMI)
// X(ETH)
// X(FSMC)
// X(SDIO)
// X(RNG) X(LCD)
// X(HDMI_CEC)
// X(COMP1) X(COMP2) X(COMP3) X(COMP4) X(COMP5) X(COMP6) X(COMP7)

// GPIO resources have names generated dynamically to save ROM
#define XX_RESOURCES_GPIO \
    X(PA0) X(PA1) X(PA2) X(PA3) X(PA4) X(PA5) X(PA6) X(PA7) \
    X(PA8) X(PA9) X(PA10) X(PA11) X(PA12) X(PA13) X(PA14) X(PA15) \
    X(PB0) X(PB1) X(PB2) X(PB3) X(PB4) X(PB5) X(PB6) X(PB7) \
    X(PB8) X(PB9) X(PB10) X(PB11) X(PB12) X(PB13) X(PB14) X(PB15) \
    X(PC0) X(PC1) X(PC2) X(PC3) X(PC4) X(PC5) X(PC6) X(PC7) \
    X(PC8) X(PC9) X(PC10) X(PC11) X(PC12) X(PC13) X(PC14) X(PC15) \
    X(PD0) X(PD1) X(PD2) X(PD3) X(PD4) X(PD5) X(PD6) X(PD7) \
    X(PD8) X(PD9) X(PD10) X(PD11) X(PD12) X(PD13) X(PD14) X(PD15) \
    X(PE0) X(PE1) X(PE2) X(PE3) X(PE4) X(PE5) X(PE6) X(PE7) \
    X(PE8) X(PE9) X(PE10) X(PE11) X(PE12) X(PE13) X(PE14) X(PE15) \
    X(PF0) X(PF1) X(PF2) X(PF3) X(PF4) X(PF5) X(PF6) X(PF7) \
    X(PF8) X(PF9) X(PF10) X(PF11) X(PF12) X(PF13) X(PF14) X(PF15) \

// EXTI resources have names generated dynamically to save ROM
#define XX_RESOURCES_EXTI \
    X(EXTI0) X(EXTI1) X(EXTI2) X(EXTI3) X(EXTI4) X(EXTI5) X(EXTI6) X(EXTI7) \
    X(EXTI8) X(EXTI9) X(EXTI10) X(EXTI11) X(EXTI12) X(EXTI13) X(EXTI14) X(EXTI15)

/** Resource typedef for cleaner code */
typedef enum hw_resource Resource;

/** Enum of all resources */
enum hw_resource {
#define X(res_name) R_##res_name,
    // GPIO are at the beginning, because some units use the constants in their config to represent
    // selected pins and those must not change with adding more stuff to the main list
    XX_RESOURCES_GPIO
    // EXTIs (same like GPIOs) have dynamically generated labels to save rom space. Must be contiguous.
    XX_RESOURCES_EXTI
    // All the rest ...
    XX_RESOURCES
#undef X
    R_NONE,
    RESOURCE_COUNT = R_NONE,
};

/** Length of the resource map */
#define RSCMAP_LEN ((RESOURCE_COUNT/8)+1)

/**
 * Resource map is a bitfield byte array located in all component instances.
 * Each bit corresponds to one resource claimed by the unit.
 */
typedef uint8_t ResourceMap[RSCMAP_LEN];

/**
 * Check if a resource is free
 *
 * @param rscmap - resource map to look into
 * @param rsc - tested resource
 * @return is free
 */
static inline bool rscmap_is_free(const ResourceMap *rscmap, Resource rsc)
{
    return (0 == ((*rscmap)[(rsc >> 3) & 0xFF] & (1 << (rsc & 0x7))));
}

/**
 * Check if a resource is held
 *
 * @param rscmap - resource map to look into
 * @param rsc - tested resource
 * @return is not free
 */
static inline bool rscmap_is_held(const ResourceMap *rscmap, Resource rsc)
{
    return !rscmap_is_free(rscmap, rsc);
}

/**
 * Claim a resource in a resource map
 *
 * @param rscmap - resource map to modify
 * @param rsc - resource to claim
 */
static inline void rscmap_claim(ResourceMap *rscmap, Resource rsc)
{
    (*rscmap)[(rsc >> 3) & 0xFF] |= (1 << (rsc & 0x7));
}

/**
 * Release a resource in a resource map
 *
 * @param rscmap - resource map to modify
 * @param rsc - resource to release
 */
static inline void rscmap_free(ResourceMap *rscmap, Resource rsc)
{
    (*rscmap)[(rsc >> 3) & 0xFF] &= ~(1 << (rsc & 0x7));
}

// helper macros
#define RSC_IS_FREE(rscmap, rsc) rscmap_is_free(&rscmap, (rsc))
#define RSC_IS_HELD(rscmap, rsc) rscmap_is_held(&rscmap, (rsc))
#define RSC_CLAIM(rscmap, rsc)   rscmap_claim(&rscmap, (rsc))
#define RSC_FREE(rscmap, rsc)    rscmap_free(&rscmap, (rsc))

#endif //GEX_CORE_RSC_ENUM_H
