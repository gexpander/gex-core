//
// Created by MightyPork on 2017/12/15.
//

#include "platform.h"
#include "framework/resources.h"
#include "debug_uart.h"
#include "plat_compat.h"

#if USE_DEBUG_UART

/** Init the submodule. */
void DebugUart_Init(void)
{
    // Debug UART
    bool ok = true;
    ok &= rsc_claim(&UNIT_SYSTEM, R_USART2);
    ok &= rsc_claim(&UNIT_SYSTEM, R_PA2);
    assert_param(ok);
}

/** Init the hardware peripheral - this is called early in the boot process */
void DebugUart_PreInit(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_2, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_2, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_2, LL_GPIO_SPEED_FREQ_HIGH);

    // commented out default values
//    LL_USART_ConfigAsyncMode(USART2);
//    LL_USART_SetDataWidth(USART2, LL_USART_DATAWIDTH_8B);
//    LL_USART_SetParity(USART2, LL_USART_PARITY_NONE);
//    LL_USART_SetStopBitsLength(USART2, LL_USART_STOPBITS_1);
//    LL_USART_SetHWFlowCtrl(USART2, LL_USART_HWCONTROL_NONE);
    LL_USART_EnableDirectionTx(USART2);

#if GEX_PLAT_F072_DISCOVERY
    LL_USART_SetBaudRate(USART2, SystemCoreClock, LL_USART_OVERSAMPLING_16, 115200); // This is not great, let's hope it's like this on all platforms...
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_2, LL_GPIO_AF_1);
#elif GEX_PLAT_F103_BLUEPILL
    LL_USART_SetBaudRate(USART2, SystemCoreClock/2, 115200); // This is not great, let's hope it's like this on all platforms...
#else
    #error "BAD PLATFORM!"
#endif

    LL_USART_Enable(USART2);
}

/** Debug print, used by debug / newlib */
ssize_t _write_r(struct _reent *rptr, int fd, const void *buf, size_t len)
{
    (void)rptr;

    uint8_t *buff = buf;

    for (uint32_t i = 0; i < len; i++) {
        while (!LL_USART_IsActiveFlag_TC(USART2));
        LL_USART_TransmitData8(USART2, *buff++);
    }

    return len;
}

#else

// No-uart variant
void DebugUart_Init(void) {}
ssize_t _write_r(struct _reent *rptr, int fd, const void *buf, size_t len) {}

#endif //USE_DEBUG_UART
