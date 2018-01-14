//
// Created by MightyPork on 2018/01/14.
//
#include <stm32f0xx_ll_dma.h>
#include <stm32f072xb.h>
#include <platform/irq_dispatcher.h>
#include "platform.h"
#include "unit_base.h"

#define UUSART_INTERNAL
#include "_internal.h"

static void UUSART_DMA_RxHandler(void *arg);
static void UUSART_DMA_TxHandler(void *arg);

error_t UUSART_ClaimDMAs(Unit *unit)
{
    error_t rv;
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    priv->dma = DMA1;

    switch (priv->periph_num) {
        /* USART1 */
        case 1:
            // TX
            rv = rsc_claim(unit, R_DMA1_2);
            if (rv == E_SUCCESS) {
                LL_SYSCFG_SetRemapDMA_USART(LL_SYSCFG_USART1TX_RMP_DMA1CH2);
                priv->dma_tx = DMA1_Channel2;
                priv->dma_tx_chnum = 2;
            } else {
                // try the remap
                TRY(rsc_claim(unit, R_DMA1_4));
                LL_SYSCFG_SetRemapDMA_USART(LL_SYSCFG_USART1TX_RMP_DMA1CH4);
                priv->dma_tx = DMA1_Channel4;
                priv->dma_tx_chnum = 4;
            }

            // RX
            rv = rsc_claim(unit, R_DMA1_3);
            if (rv == E_SUCCESS) {
                LL_SYSCFG_SetRemapDMA_USART(LL_SYSCFG_USART1RX_RMP_DMA1CH3);
                priv->dma_rx = DMA1_Channel3;
                priv->dma_rx_chnum = 3;
            } else {
                // try the remap
                TRY(rsc_claim(unit, R_DMA1_5));
                LL_SYSCFG_SetRemapDMA_USART(LL_SYSCFG_USART1RX_RMP_DMA1CH5);
                priv->dma_rx = DMA1_Channel5;
                priv->dma_rx_chnum = 5;
            }
            break;

        /* USART2 */
        case 2:
            // RX,TX
            rv = rsc_claim_range(unit, R_DMA1_4, R_DMA1_5);
            if (rv == E_SUCCESS) {
                LL_SYSCFG_SetRemapDMA_USART(LL_SYSCFG_USART2_RMP_DMA1CH54);
                priv->dma_tx = DMA1_Channel4;
                priv->dma_rx = DMA1_Channel5;
                priv->dma_tx_chnum = 4;
                priv->dma_rx_chnum = 5;
            } else {
                // try the remap
                TRY(rsc_claim_range(unit, R_DMA1_6, R_DMA1_7));
                LL_SYSCFG_SetRemapDMA_USART(LL_SYSCFG_USART2_RMP_DMA1CH67);
                priv->dma_tx = DMA1_Channel7;
                priv->dma_rx = DMA1_Channel6;
                priv->dma_tx_chnum = 7;
                priv->dma_rx_chnum = 6;
            }
            break;

        /* USART3 */
        case 3:
            // RX,TX
            rv = rsc_claim_range(unit, R_DMA1_6, R_DMA1_7);
            if (rv == E_SUCCESS) {
                LL_SYSCFG_SetRemapDMA_USART(LL_SYSCFG_USART3_RMP_DMA1CH67);
                priv->dma_tx = DMA1_Channel7;
                priv->dma_rx = DMA1_Channel6;
                priv->dma_tx_chnum = 7;
                priv->dma_rx_chnum = 6;
            } else {
                // try the remap
                TRY(rsc_claim_range(unit, R_DMA1_2, R_DMA1_3));
                LL_SYSCFG_SetRemapDMA_USART(LL_SYSCFG_USART3_RMP_DMA1CH32);
                priv->dma_tx = DMA1_Channel2;
                priv->dma_rx = DMA1_Channel3;
                priv->dma_tx_chnum = 2;
                priv->dma_rx_chnum = 3;
            }
            break;

        /* USART4 */
        case 4:
            // RX,TX
            TRY(rsc_claim_range(unit, R_DMA1_6, R_DMA1_7));
            priv->dma_tx = DMA1_Channel7;
            priv->dma_rx = DMA1_Channel6;
            priv->dma_tx_chnum = 7;
            priv->dma_rx_chnum = 6;
            break;

        default:
            trap("Missing DMA mapping for USART%d", (int)priv->periph_num);
    }

    dbg("USART %d - selected DMA ch Tx(%d), Rx(%d)", priv->periph_num, priv->dma_tx_chnum, priv->dma_rx_chnum);

    return E_SUCCESS;
}

error_t UUSART_SetupDMAs(Unit *unit)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    priv->rx_buffer = malloc_ck(UUSART_RXBUF_LEN);
    if (NULL == priv->rx_buffer) return E_OUT_OF_MEM;

    priv->tx_buffer = malloc_ck(UUSART_TXBUF_LEN);
    if (NULL == priv->tx_buffer) return E_OUT_OF_MEM;

    priv->rx_buf_readpos = 0;

    LL_DMA_InitTypeDef init;

    // Transmit buffer
    {
        LL_DMA_StructInit(&init);
        init.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
        init.Mode = LL_DMA_MODE_CIRCULAR;

        init.PeriphOrM2MSrcAddress = (uint32_t) &priv->periph->TDR;
        init.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
        init.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;

        init.MemoryOrM2MDstAddress = (uint32_t) priv->tx_buffer;
        init.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
        init.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;

        assert_param(SUCCESS == LL_DMA_Init(priv->dma, priv->dma_tx_chnum, &init));

        irqd_attach(priv->dma_tx, UUSART_DMA_TxHandler, unit);
        // Interrupt on transfer complete
        LL_DMA_EnableIT_TC(priv->dma, priv->dma_tx_chnum);
    }

    // Receive buffer
    {
        LL_DMA_StructInit(&init);
        init.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;

        init.Mode = LL_DMA_MODE_CIRCULAR;
        init.NbData = UUSART_RXBUF_LEN;

        init.PeriphOrM2MSrcAddress = (uint32_t) &priv->periph->RDR;
        init.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
        init.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;

        init.MemoryOrM2MDstAddress = (uint32_t) priv->rx_buffer;
        init.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
        init.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;

        assert_param(SUCCESS == LL_DMA_Init(priv->dma, priv->dma_rx_chnum, &init));

        irqd_attach(priv->dma_rx, UUSART_DMA_RxHandler, unit);
        // Interrupt on transfer 1/2 and complete
        // We will capture the first and second half and send it while the other half is being filled.
        LL_DMA_EnableIT_HT(priv->dma, priv->dma_rx_chnum);
        LL_DMA_EnableIT_TC(priv->dma, priv->dma_rx_chnum);
    }

    LL_DMA_EnableChannel(priv->dma, priv->dma_rx_chnum);
    LL_DMA_EnableChannel(priv->dma, priv->dma_tx_chnum);

    // TODO also set up usart timeout interrupt that grabs whatever is in the DMA buffer and sends it
    return E_SUCCESS;
}

static void UUSART_DMA_RxHandler(void *arg)
{
    Unit *unit = arg;
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    uint32_t isrsnapshot = priv->dma->ISR;

    if (LL_DMA_IsActiveFlag_G(isrsnapshot, priv->dma_rx_chnum)) {
        bool tc = LL_DMA_IsActiveFlag_TC(isrsnapshot, priv->dma_rx_chnum);
        bool ht = LL_DMA_IsActiveFlag_HT(isrsnapshot, priv->dma_rx_chnum);


        uint16_t end = (uint16_t) (ht ? UUSART_RXBUF_LEN / 2 : UUSART_RXBUF_LEN);

        uint16_t toRead = (uint16_t) (end - priv->rx_buf_readpos);

        PUTS(">");
        PUTSN((char *) priv->rx_buffer+priv->rx_buf_readpos, toRead);
        PUTS("<\r\n");

        // Prepare position for next read
        if (ht) priv->rx_buf_readpos = (uint16_t) (end);
        else priv->rx_buf_readpos = 0;


        if (ht) LL_DMA_ClearFlag_HT(priv->dma, priv->dma_rx_chnum);
        if (tc) LL_DMA_ClearFlag_TC(priv->dma, priv->dma_rx_chnum);
        if (LL_DMA_IsActiveFlag_TE(isrsnapshot, priv->dma_rx_chnum)) {
            dbg("Rx TE"); // this shouldn't happen
            LL_DMA_ClearFlag_TE(priv->dma, priv->dma_rx_chnum);
        }
    }
}

static void UUSART_DMA_TxHandler(void *arg)
{
    Unit *unit = arg;
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    uint32_t isrsnapshot = priv->dma->ISR;
    if (LL_DMA_IsActiveFlag_G(isrsnapshot, priv->dma_tx_chnum)) {
        if (LL_DMA_IsActiveFlag_TC(isrsnapshot, priv->dma_tx_chnum)) {
            dbg("Tx TC");
        }

        LL_DMA_ClearFlags(priv->dma, priv->dma_tx_chnum);
    }
}


void UUSART_DeInitDMAs(Unit *unit)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    irqd_detach(priv->dma_tx, UUSART_DMA_RxHandler);
    irqd_detach(priv->dma_rx, UUSART_DMA_TxHandler);

    LL_DMA_DeInit(priv->dma, priv->dma_rx_chnum);
    LL_DMA_DeInit(priv->dma, priv->dma_tx_chnum);

    free_ck(priv->rx_buffer);
    free_ck(priv->tx_buffer);
}
