//
// Created by MightyPork on 2018/01/06.
//

#ifndef GEX_F072_RSC_ENUM_H
#define GEX_F072_RSC_ENUM_H

// X macro: Resource name,
#define XX_RESOURCES \
    X(SPI1) X(SPI2) X(SPI3) \
    X(I2C1) X(I2C2) X(I2C3) \
    X(ADC1) X(ADC2) X(ADC3) X(ADC4) \
    X(DAC1) X(DAC2) \
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
// X(TSC)
// X(DCMI)
// X(ETH)
// X(FSMC)
// X(SDIO)
// X(RNG) X(LCD)
// X(HDMI_CEC)
// X(COMP1) X(COMP2) X(COMP3) X(COMP4) X(COMP5) X(COMP6) X(COMP7)

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

// GPIOs are allocated whenever the pin is needed
//  (e.g. when used for SPI, the R_SPI resource as well as the corresponding R_GPIO resources must be claimed)

// Peripheral blocks (IPs) - not all chips have all blocks, usually the 1 and 2 are present as a minimum, if any.
//  It doesn't really make sense to expose multiple instances of buses that support addressing

// ADCs - some more advanced chips support differential input mode on some (not all!) inputs
//  Usually only one or two instances are present

// DAC - often only one is present, or none.

// UARTs
//  - 1 and 2 are present universally, 2 is connected to VCOM on Nucleo/Discovery boards, good for debug messages
//  4 and 5 don't support synchronous mode.

// Timers
//  - some support quadrature input, probably all support external clock / gating / clock-out/PWM generation
//  Not all chips have all timers and not all timers are equal.

// DMA - Direct memory access lines - TODO split those to channels, they can be used separately

// The resource registry will be pre-loaded with platform-specific config of which blocks are available - the rest will be "pre-claimed"
// (i.e. unavailable to functional modules)

typedef enum hw_resource Resource;

enum hw_resource {
#define X(res_name) R_##res_name,
    XX_RESOURCES
    XX_RESOURCES_GPIO
#undef X
    R_NONE,
    RESOURCE_COUNT = R_NONE,
};

#define RSCMAP_LEN ((RESOURCE_COUNT/8)+1)

typedef uint8_t ResourceMap[RSCMAP_LEN];

#define RSC_IS_FREE(rscmap, rsc) (0 == (rscmap[((rsc)>>3)&0xFF] & (1<<((rsc)&0x7))))
#define RSC_IS_HELD(rscmap, rsc) (!RSC_IS_FREE(rscmap, rsc))
#define RSC_CLAIM(rscmap, rsc)   do { rscmap[((rsc)>>3)&0xFF] |= (1<<((rsc)&0x7)); } while(0)
#define RSC_FREE(rscmap, rsc)   do { rscmap[((rsc)>>3)&0xFF] &= ~(1<<((rsc)&0x7)); } while(0)


#endif //GEX_F072_RSC_ENUM_H
