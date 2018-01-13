//
// Created by MightyPork on 2017/12/15.
//

#include "platform.h"
#include "framework/resources.h"
#include "debug_uart.h"
#include "plat_compat.h"
#include "pin_utils.h"

#if USE_DEBUG_UART

#define DEBUG_USART_BAUD 115200

#if GEX_PLAT_F072_DISCOVERY

    #define DEBUG_USART USART1
    #define DEBUG_USART_RSC R_USART1
    #define DEBUG_USART_PORT 'A'
    #define DEBUG_USART_PIN  9
    #define DEBUG_USART_AF  1
    #define DEBUG_USART_PCLK PLAT_APB1_HZ

#elif GEX_PLAT_F103_BLUEPILL

    #define DEBUG_USART USART2
    #define DEBUG_USART_RSC R_USART2
    #define DEBUG_USART_PORT 'A'
    #define DEBUG_USART_PIN  2
    #define DEBUG_USART_PCLK PLAT_APB1_HZ

#elif GEX_PLAT_F303_DISCOVERY

    #define DEBUG_USART USART3
    #define DEBUG_USART_RSC R_USART3
    #define DEBUG_USART_PORT 'D'
    #define DEBUG_USART_PIN  8
    #define DEBUG_USART_AF  7
    #define DEBUG_USART_PCLK PLAT_APB1_HZ

#elif GEX_PLAT_F407_DISCOVERY

    #define DEBUG_USART USART2
    #define DEBUG_USART_RSC R_USART2
    #define DEBUG_USART_PORT 'A'
    #define DEBUG_USART_PIN  2
    #define DEBUG_USART_AF  7
    #define DEBUG_USART_PCLK PLAT_APB1_HZ

#else
    #error "BAD PLATFORM!"
#endif



/** Init the submodule. */
void DebugUart_Init(void)
{
    bool suc = true;
    // Debug UART
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, DEBUG_USART_RSC));
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, pin2resource(DEBUG_USART_PORT, DEBUG_USART_PIN, &suc)));
    assert_param(suc);
}

/** Init the hardware peripheral - this is called early in the boot process */
void DebugUart_PreInit(void)
{
    // configure AF only if platform uses AF numbers
#if !PLAT_NO_AFNUM
    configure_gpio_alternate(DEBUG_USART_PORT, DEBUG_USART_PIN, DEBUG_USART_AF);
#endif

    if (DEBUG_USART == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
    }
    else if (DEBUG_USART == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
    }
    else if (DEBUG_USART == USART3) {
        __HAL_RCC_USART3_CLK_ENABLE();
    }
#ifdef USART4
    else if (DEBUG_USART == USART4) {
        __HAL_RCC_USART4_CLK_ENABLE();
    }
#endif
#ifdef USART5
    else if (DEBUG_USART == USART5) {
        __HAL_RCC_USART5_CLK_ENABLE();
    }
#endif

//    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_2, LL_GPIO_MODE_ALTERNATE);
//    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_2, LL_GPIO_OUTPUT_PUSHPULL);
//    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_2, LL_GPIO_SPEED_FREQ_HIGH);

    // commented out default values
//    LL_USART_ConfigAsyncMode(USART2);
//    LL_USART_SetDataWidth(USART2, LL_USART_DATAWIDTH_8B);
//    LL_USART_SetParity(USART2, LL_USART_PARITY_NONE);
//    LL_USART_SetStopBitsLength(USART2, LL_USART_STOPBITS_1);
//    LL_USART_SetHWFlowCtrl(USART2, LL_USART_HWCONTROL_NONE);
    LL_USART_EnableDirectionTx(DEBUG_USART);
    LL_USART_SetBaudRate(DEBUG_USART, DEBUG_USART_PCLK, LL_USART_OVERSAMPLING_16, DEBUG_USART_BAUD);
    LL_USART_Enable(DEBUG_USART);
}

void debug_write(const char *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        while (!LL_USART_IsActiveFlag_TC(DEBUG_USART));
        LL_USART_TransmitData8(DEBUG_USART, (uint8_t) *buf++);
    }
}

/** Debug print, used by debug / newlib */
ssize_t _write_r(struct _reent *rptr, int fd, const void *buf, size_t len)
{
    trap("Use of newlib printf");
    return len;
}

#else

// No-uart variant
void DebugUart_PreInit(void) {}
void DebugUart_Init(void) {}
ssize_t _write_r(struct _reent *rptr, int fd, const void *buf, size_t len) {
    return len;
}

#endif //USE_DEBUG_UART
