//
// Created by MightyPork on 2017/11/26.
//

#include "platform.h"
#include "hw_utils.h"

/** Convert pin number to LL bitfield */
uint32_t hw_pin2ll(uint8_t pin_number, bool *suc)
{
    assert_param(suc != NULL);

    if(pin_number > 15) {
        dbg("Bad pin: %d", pin_number);
        // TODO proper report
        *suc = false;
        return 0;
    }
    return LL_GPIO_PINS[pin_number];
}

/** Convert port name (A,B,C...) to peripheral struct pointer */
GPIO_TypeDef *hw_port2periph(char port_name, bool *suc)
{
    assert_param(suc != NULL);

    if(port_name < 'A' || port_name >= ('A'+PORTS_COUNT)) {
        dbg("Bad port: %c", port_name); // TODO proper report
        *suc = false;
        return NULL;
    }

    uint8_t num = (uint8_t) (port_name - 'A');
    return GPIO_PERIPHS[num];
}

/** Convert a pin resource to it's LL lib values */
bool hw_pinrsc2ll(Resource rsc, GPIO_TypeDef **port, uint32_t *llpin)
{
    if (rsc > R_PF15) return false;
    uint32_t index = rsc - R_PA0;
    uint32_t pname = (index/16);
    uint8_t pnum = (uint8_t) (index % 16);
    if (pname >= PORTS_COUNT) return false;
    *port = GPIO_PERIPHS[pname];
    *llpin = LL_GPIO_PINS[pnum];
    return true;
}

#pragma GCC push_options
#pragma GCC optimize ("O2")
/** spread a packed pinfield using a mask */
uint32_t pinmask_spread_32(uint32_t packed, uint32_t mask)
{
    uint32_t result = 0;
    uint32_t poke = 1;
    if(packed == 0) return 0;

    for (int i = 0; i < 32; i++) {
        if (mask & 1) {
            if (packed & poke) {
                result |= 1<<i;
                packed ^= poke;
                if (packed == 0) break;
            }
            poke <<= 1;
        }
        mask >>= 1;
        // if (mask == 0) break;
    }

    return result;
}

/** Pack spread port pins using a mask */
uint32_t pinmask_pack_32(uint32_t spread, uint32_t mask)
{
    uint32_t result = 0;
    uint32_t poke = 1;
    for (int i = 0; i<32; i++) {
        if (mask & 1) {
            if (spread & (1<<i)) {
                result |= poke;
                spread ^= (1<<i);
                if (spread == 0) break;
            }
            poke <<= 1;
        }
        mask >>= 1;
        // if (mask == 0) break;
    }
    return result;
}

#pragma GCC pop_options

/** Configure unit pins as analog (part of unit teardown) */
void hw_deinit_unit_pins(Unit *unit)
{
    for (uint32_t rsc = R_PA0; rsc <= R_PF15; rsc++) {
        if (RSC_IS_HELD(unit->resources, (Resource)rsc)) {
            hw_deinit_pin_rsc((Resource)rsc);
        }
    }
}

void hw_deinit_pin_rsc(Resource rsc)
{
    rsc_dbg("Freeing pin %s", rsc_get_name((Resource)rsc));
    GPIO_TypeDef *port = GPIO_PERIPHS[(rsc-R_PA0) / 16];
    uint32_t ll_pin = LL_GPIO_PINS[(rsc-R_PA0)%16];
    LL_GPIO_SetPinMode(port, ll_pin, LL_GPIO_MODE_ANALOG);
}

/** Configure a pin to alternate function */
error_t hw_configure_gpio_af(char port_name, uint8_t pin_num, uint32_t ll_af)
{
#if PLAT_NO_AFNUM
    trap("Illegal call to hw_configure_gpio_af() on this platform");
#else

    bool suc = true;
    GPIO_TypeDef *port = hw_port2periph(port_name, &suc);
    uint32_t ll_pin = hw_pin2ll(pin_num, &suc);
    if (!suc) return E_BAD_CONFIG;

    if (pin_num < 8)
        LL_GPIO_SetAFPin_0_7(port, ll_pin, ll_af);
    else
        LL_GPIO_SetAFPin_8_15(port, ll_pin, ll_af);

    LL_GPIO_SetPinMode(port, ll_pin, LL_GPIO_MODE_ALTERNATE);

#endif
    return E_SUCCESS;
}

/** Configure a pin to alternate function */
error_t hw_configure_gpiorsc_af(Resource rsc, uint32_t ll_af)
{
#if PLAT_NO_AFNUM
    trap("Illegal call to hw_configure_gpio_af() on this platform");
#else

    bool suc = true;
    GPIO_TypeDef *port;
    uint32_t ll_pin;
    suc = hw_pinrsc2ll(rsc, &port, &ll_pin);
    if (!suc) return E_BAD_CONFIG;

    if (ll_pin & 0xFF)
        LL_GPIO_SetAFPin_0_7(port, ll_pin, ll_af);
    else
        LL_GPIO_SetAFPin_8_15(port, ll_pin, ll_af);

    LL_GPIO_SetPinMode(port, ll_pin, LL_GPIO_MODE_ALTERNATE);

#endif
    return E_SUCCESS;
}

/** Configure pins using sparse map */
error_t hw_configure_sparse_pins(char port_name, uint16_t mask, GPIO_TypeDef **port_dest,
                                 uint32_t ll_mode, uint32_t ll_otype)
{
    bool suc = true;
    GPIO_TypeDef *port = hw_port2periph(port_name, &suc);
    if (!suc) return E_BAD_CONFIG;

    for (int i = 0; i < 16; i++) {
        if (mask & (1<<i)) {
            uint32_t ll_pin = hw_pin2ll((uint8_t) i, &suc);
            LL_GPIO_SetPinMode(port, ll_pin, ll_mode);
            LL_GPIO_SetPinOutputType(port, ll_pin, ll_otype);
            LL_GPIO_SetPinSpeed(port, ll_pin, LL_GPIO_SPEED_FREQ_HIGH);
        }
    }

    if (port_dest != NULL) {
        *port_dest = port;
    }

    return E_SUCCESS;
}

/** Solve a timer/counter's count and prescaller value */
bool hw_solve_timer(uint32_t base_freq, uint32_t required_freq, bool is16bit,
                    uint16_t *presc, uint32_t *count, float *real_freq)
{
    if (required_freq == 0) return false;

    // XXX consider using the LL macros __LL_TIM_CALC_PSC and __LL_TIM_CALC_ARR

    const float fPresc = base_freq / required_freq;
    uint32_t wCount = (uint32_t) lrintf(fPresc);

    const uint32_t ceil = is16bit ? UINT16_MAX : UINT32_MAX;

    uint32_t wPresc = 1;
    while (wCount > ceil) {
        wPresc <<= 1;
        wCount >>= 1;
    }

    if (wPresc > ceil || count == 0) {
        return false;
    }

    *count = wCount;
    *presc = (uint16_t) wPresc;

    if (wPresc * wCount == 0) return false;

    if (real_freq != NULL) {
        *real_freq = (base_freq / (wPresc * wCount));
    }

    return true;
}

void hw_periph_clock_enable(void *periph)
{
    // GPIOs are enabled by default on start-up

    // --- USART ---
    if (periph == USART1) __HAL_RCC_USART1_CLK_ENABLE();
    else if (periph == USART2) __HAL_RCC_USART2_CLK_ENABLE();
#ifdef USART3
    else if (periph == USART3) __HAL_RCC_USART3_CLK_ENABLE();
#endif
#ifdef USART4
    else if (periph == USART4) __HAL_RCC_USART4_CLK_ENABLE();
#endif
#ifdef USART5
    else if (periph == USART5) __HAL_RCC_USART5_CLK_ENABLE();
#endif

    // --- SPI ---
    else if (periph == SPI1) __HAL_RCC_SPI1_CLK_ENABLE();
#ifdef SPI2
    else if (periph == SPI2) __HAL_RCC_SPI2_CLK_ENABLE();
#endif
#ifdef SPI3
    else if (periph == SPI3) __HAL_RCC_SPI3_CLK_ENABLE();
#endif

    // --- I2C ---
    else if (periph == I2C1) __HAL_RCC_I2C1_CLK_ENABLE();
    else if (periph == I2C2) __HAL_RCC_I2C2_CLK_ENABLE();
#ifdef I2C3
    else if (periph == I2C3) __HAL_RCC_I2C3_CLK_ENABLE();
#endif

    // --- DMA ---
    else if (periph == DMA1) __HAL_RCC_DMA1_CLK_ENABLE();
#ifdef DMA2
    else if (periph == DMA2) __HAL_RCC_DMA2_CLK_ENABLE();
#endif

    // --- TIM ---
    else if (periph == TIM1) __HAL_RCC_TIM1_CLK_ENABLE();
    else if (periph == TIM2) __HAL_RCC_TIM2_CLK_ENABLE();
    else if (periph == TIM3) __HAL_RCC_TIM3_CLK_ENABLE();
#ifdef TIM4
    else if (periph == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
#endif
#ifdef TIM5
    else if (periph == TIM5) __HAL_RCC_TIM5_CLK_ENABLE();
#endif
#ifdef TIM6
    else if (periph == TIM6) __HAL_RCC_TIM6_CLK_ENABLE();
#endif
#ifdef TIM7
    else if (periph == TIM7) __HAL_RCC_TIM7_CLK_ENABLE();
#endif
#ifdef TIM8
    else if (periph == TIM8) __HAL_RCC_TIM8_CLK_ENABLE();
#endif
#ifdef TIM9
    else if (periph == TIM9) __HAL_RCC_TIM9_CLK_ENABLE();
#endif
#ifdef TIM11
    else if (periph == TIM11) __HAL_RCC_TIM11_CLK_ENABLE();
#endif
#ifdef TIM12
    else if (periph == TIM12) __HAL_RCC_TIM12_CLK_ENABLE();
#endif
#ifdef TIM13
    else if (periph == TIM13) __HAL_RCC_TIM13_CLK_ENABLE();
#endif
#ifdef TIM14
    else if (periph == TIM14) __HAL_RCC_TIM14_CLK_ENABLE();
#endif
#ifdef TIM15
    else if (periph == TIM15) __HAL_RCC_TIM15_CLK_ENABLE();
#endif
#ifdef TIM16
    else if (periph == TIM16) __HAL_RCC_TIM15_CLK_ENABLE();
#endif
#ifdef TIM17
    else if (periph == TIM17) __HAL_RCC_TIM17_CLK_ENABLE();
#endif

    // --- ADC ---
#ifdef ADC1
    else if (periph == ADC1) __HAL_RCC_ADC1_CLK_ENABLE();
#endif
#ifdef ADC2
    else if (periph == ADC2) __HAL_RCC_ADC2_CLK_ENABLE();
#endif

    // --- DAC ---
#ifdef DAC1
    else if (periph == DAC1) __HAL_RCC_DAC1_CLK_ENABLE();
#endif
#ifdef DAC2
    else if (periph == DAC2) __HAL_RCC_DAC2_CLK_ENABLE();
#endif

        // --- TSC ---
#ifdef TSC
    else if (periph == TSC) __HAL_RCC_TSC_CLK_ENABLE();
#endif

    else {
        dbg("Periph 0x%p missing in hw clock enable func", periph);
        trap("BUG");
    }
}


void hw_periph_clock_disable(void *periph)
{
    // GPIOs are enabled by default on start-up

    // --- USART ---
    if (periph == USART1) __HAL_RCC_USART1_CLK_DISABLE();
    else if (periph == USART2) __HAL_RCC_USART2_CLK_DISABLE();
#ifdef USART3
    else if (periph == USART3) __HAL_RCC_USART3_CLK_DISABLE();
#endif
#ifdef USART4
    else if (periph == USART4) __HAL_RCC_USART4_CLK_DISABLE();
#endif
#ifdef USART5
    else if (periph == USART5) __HAL_RCC_USART5_CLK_DISABLE();
#endif

        // --- SPI ---
    else if (periph == SPI1) __HAL_RCC_SPI1_CLK_DISABLE();
#ifdef SPI2
    else if (periph == SPI2) __HAL_RCC_SPI2_CLK_DISABLE();
#endif
#ifdef SPI3
    else if (periph == SPI3) __HAL_RCC_SPI3_CLK_DISABLE();
#endif

        // --- I2C ---
    else if (periph == I2C1) __HAL_RCC_I2C1_CLK_DISABLE();
    else if (periph == I2C2) __HAL_RCC_I2C2_CLK_DISABLE();
#ifdef I2C3
    else if (periph == I2C3) __HAL_RCC_I2C3_CLK_DISABLE();
#endif

        // --- DMA ---
    else if (periph == DMA1) __HAL_RCC_DMA1_CLK_DISABLE();
#ifdef DMA2
    else if (periph == DMA2) __HAL_RCC_DMA2_CLK_DISABLE();
#endif

        // --- TIM ---
    else if (periph == TIM1) __HAL_RCC_TIM1_CLK_DISABLE();
    else if (periph == TIM2) __HAL_RCC_TIM2_CLK_DISABLE();
    else if (periph == TIM3) __HAL_RCC_TIM3_CLK_DISABLE();
#ifdef TIM4
    else if (periph == TIM4) __HAL_RCC_TIM4_CLK_DISABLE();
#endif
#ifdef TIM5
    else if (periph == TIM5) __HAL_RCC_TIM5_CLK_DISABLE();
#endif
#ifdef TIM6
    else if (periph == TIM6) __HAL_RCC_TIM6_CLK_DISABLE();
#endif
#ifdef TIM7
    else if (periph == TIM7) __HAL_RCC_TIM7_CLK_DISABLE();
#endif
#ifdef TIM8
    else if (periph == TIM8) __HAL_RCC_TIM8_CLK_DISABLE();
#endif
#ifdef TIM9
    else if (periph == TIM9) __HAL_RCC_TIM9_CLK_DISABLE();
#endif
#ifdef TIM11
    else if (periph == TIM11) __HAL_RCC_TIM11_CLK_DISABLE();
#endif
#ifdef TIM12
    else if (periph == TIM12) __HAL_RCC_TIM12_CLK_DISABLE();
#endif
#ifdef TIM13
    else if (periph == TIM13) __HAL_RCC_TIM13_CLK_DISABLE();
#endif
#ifdef TIM14
    else if (periph == TIM14) __HAL_RCC_TIM14_CLK_DISABLE();
#endif
#ifdef TIM15
    else if (periph == TIM15) __HAL_RCC_TIM15_CLK_DISABLE();
#endif
#ifdef TIM16
    else if (periph == TIM16) __HAL_RCC_TIM15_CLK_DISABLE();
#endif
#ifdef TIM17
    else if (periph == TIM17) __HAL_RCC_TIM17_CLK_DISABLE();
#endif

        // --- ADC ---
#ifdef ADC1
    else if (periph == ADC1) __HAL_RCC_ADC1_CLK_DISABLE();
#endif
#ifdef ADC2
    else if (periph == ADC2) __HAL_RCC_ADC2_CLK_DISABLE();
#endif

        // --- DAC ---
#ifdef DAC1
    else if (periph == DAC1) __HAL_RCC_DAC1_CLK_DISABLE();
#endif
#ifdef DAC2
        else if (periph == DAC2) __HAL_RCC_DAC2_CLK_DISABLE();
#endif

        // --- TSC ---
#ifdef TSC
    else if (periph == TSC) __HAL_RCC_TSC_CLK_DISABLE();
#endif

    else {
        dbg("Periph 0x%p missing in hw clock disable func", periph);
        trap("BUG");
    }
}
