//
// Created by MightyPork on 2018/04/06.
//

#include "platform.h"
#include "iface_uart.h"
#include "resources.h"
#include "hw_utils.h"
#include "irq_dispatcher.h"
#include "task_msg.h"
#include "system_settings.h"

extern osSemaphoreId semVcomTxReadyHandle;

static uint32_t usart_rxi = 0;
static uint8_t usart_rx_buffer[MSG_QUE_SLOT_SIZE];
static uint32_t last_rx_time = 0;

void iface_uart_claim_resources(void)
{
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, R_USART2));
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, R_PA2));
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, R_PA3));
}

void iface_uart_free_resources(void)
{
    rsc_free(&UNIT_SYSTEM, R_USART2);
    rsc_free(&UNIT_SYSTEM, R_PA2);
    rsc_free(&UNIT_SYSTEM, R_PA3);
}

/** Handler for the USART transport */
static void com_UsartIrqHandler(void *arg)
{
    (void)arg;
    if (LL_USART_IsActiveFlag_RXNE(USART2)) {
        vPortEnterCritical();
        {
            usart_rx_buffer[usart_rxi++] = LL_USART_ReceiveData8(USART2);
            if (usart_rxi == MSG_QUE_SLOT_SIZE) {
                rxQuePostMsg(usart_rx_buffer, MSG_QUE_SLOT_SIZE); // avoid it happening in the irq
                usart_rxi = 0;
            }
            last_rx_time = HAL_GetTick();
        }
        vPortExitCritical();
    }
}

/** this is called from the hal tick irq */
void com_iface_flush_buffer(void)
{
    if (usart_rxi > 0 && (HAL_GetTick()-last_rx_time)>=2) {
        vPortEnterCritical();
        {
            rxQuePostMsg(usart_rx_buffer, usart_rxi);
            usart_rxi = 0;
        }
        vPortExitCritical();
    }
}

bool iface_uart_init(void)
{
    dbg("Setting up UART transfer");
    assert_param(E_SUCCESS == hw_configure_gpiorsc_af(R_PA2, LL_GPIO_AF_1));
    assert_param(E_SUCCESS == hw_configure_gpiorsc_af(R_PA3, LL_GPIO_AF_1));

    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_USART2_FORCE_RESET();
    __HAL_RCC_USART2_RELEASE_RESET();

    LL_USART_Disable(USART2);

    LL_USART_SetBaudRate(USART2, PLAT_APB1_HZ, LL_USART_OVERSAMPLING_16, SystemSettings.comm_uart_baud);
    dbg("baud = %d", (int)SystemSettings.comm_uart_baud);

    irqd_attach(USART2, com_UsartIrqHandler, NULL);
    LL_USART_EnableIT_RXNE(USART2);
    LL_USART_SetTransferDirection(USART2, LL_USART_DIRECTION_TX_RX);

    LL_USART_Enable(USART2);

    return true; // always OK (TODO check voltage on Rx if it's 3V3 when idle?)
}

void iface_uart_deinit(void)
{
    // this doesn't normally happen
    hw_deinit_pin_rsc(R_PA2);
    hw_deinit_pin_rsc(R_PA3);
    __HAL_RCC_USART2_CLK_DISABLE();
    irqd_detach(USART2, com_UsartIrqHandler);
}

void iface_uart_transmit(const uint8_t *buff, uint32_t len)
{
    // TODO rewrite this to use DMA, then wait for the DMA
    for(uint32_t i=0;i<len;i++) {
        while(!LL_USART_IsActiveFlag_TXE(USART2));
        LL_USART_TransmitData8(USART2, buff[i]);
    }
    xSemaphoreGive(semVcomTxReadyHandle); // act as if we just finished it and this is perhaps the DMA irq
}
