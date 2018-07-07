//
// Created by MightyPork on 2017/12/15.
//

#include "platform.h"
#include "framework/resources.h"
#include "debug_uart.h"
#include "hw_utils.h"
#include "framework/system_settings.h"

#if USE_DEBUG_UART

static bool debug_uart_inited = false;
static bool debug_uart_preinited = false;

/** Init the submodule. */
void DebugUart_Init(void)
{
    if (debug_uart_inited) return;
    if (!debug_uart_preinited) DebugUart_PreInit();
    // Debug UART
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, DEBUG_USART_RSC));
    assert_param(E_SUCCESS == rsc_claim_pin(&UNIT_SYSTEM, DEBUG_USART_PORT, DEBUG_USART_PIN));

    debug_uart_inited = true;
}

/** Init the hardware peripheral - this is called early in the boot process */
void DebugUart_PreInit(void)
{
    debug_uart_preinited = true;

    // configure AF only if platform uses AF numbers
#if !PLAT_NO_AFNUM
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    (void) hw_configure_gpio_af(DEBUG_USART_PORT, DEBUG_USART_PIN, DEBUG_USART_AF);
#pragma GCC diagnostic pop
#endif

    hw_periph_clock_enable(DEBUG_USART);

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

void DebugUart_Teardown(void)
{
    if (!debug_uart_inited) return;

    dbg("Disabling debug UART!");

    // TODO wait for Tx (after debug print DMA is implemented)

    LL_USART_Disable(DEBUG_USART);
    rsc_free(&UNIT_SYSTEM, DEBUG_USART_RSC);
    hw_periph_clock_disable(DEBUG_USART);

    bool suc = true;
    Resource r = rsc_portpin2rsc(DEBUG_USART_PORT, DEBUG_USART_PIN, &suc);
    rsc_free(&UNIT_SYSTEM, r);
    hw_deinit_pin_rsc(r);

    debug_uart_preinited = false;
    debug_uart_inited = false;

    assert_param(suc);
}

void debug_write(const char *buf, uint16_t len)
{
    if (!SystemSettings.enable_debug_uart) return;

    // TODO wait for DMA complete
    // TODO use DMA
    for (uint16_t i = 0; i < len; i++) {
        while (!LL_USART_IsActiveFlag_TC(DEBUG_USART));
        LL_USART_TransmitData8(DEBUG_USART, (uint8_t) *buf++);
    }
}

/** Debug print, used by debug / newlib */
ssize_t _write_r(struct _reent *rptr, int fd, const void *buf, size_t len)
{
    trap("Use of newlib printf");
}

#else

// No-uart variant
void DebugUart_PreInit(void) {}
void DebugUart_Init(void) {}
ssize_t _write_r(struct _reent *rptr, int fd, const void *buf, size_t len) {
    return len;
}

#endif //USE_DEBUG_UART
