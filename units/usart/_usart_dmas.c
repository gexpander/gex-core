//
// Created by MightyPork on 2018/01/14.
//

#include "platform.h"
#include "irq_dispatcher.h"
#include "unit_base.h"

#define UUSART_INTERNAL
#include "_usart_internal.h"

static void UUSART_DMA_RxHandler(void *arg);
static void UUSART_DMA_TxHandler(void *arg);

#if UUSART_DEBUG
#define dbg_uusart(fmt, ...) dbg(fmt, ##__VA_ARGS__)
#else
#define dbg_uusart(fmt, ...)
#endif

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

    dbg_uusart("USART %d - selected DMA ch Tx(%d), Rx(%d)", priv->periph_num, priv->dma_tx_chnum, priv->dma_rx_chnum);

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

    // Those must be aligned to a word boundary for the DMAs to work.
    // Any well-behaved malloc impl should do this correctly.
    assert_param(((uint32_t)priv->rx_buffer & 3) == 0);
    assert_param(((uint32_t)priv->tx_buffer & 3) == 0);

    priv->rx_buf_readpos = 0;

    LL_DMA_InitTypeDef init;

    // Transmit buffer
    {
        LL_DMA_StructInit(&init);
        init.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
        init.Mode = LL_DMA_MODE_NORMAL;

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

    return E_SUCCESS;
}

/**
 * Handler for the Rx DMA half or full interrupt
 * @param arg - unit instance
 */
static void UUSART_DMA_RxHandler(void *arg)
{
    Unit *unit = arg;
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    const uint32_t isrsnapshot = priv->dma->ISR;

    if (LL_DMA_IsActiveFlag_G(isrsnapshot, priv->dma_rx_chnum)) {
        bool tc = LL_DMA_IsActiveFlag_TC(isrsnapshot, priv->dma_rx_chnum);
        bool ht = LL_DMA_IsActiveFlag_HT(isrsnapshot, priv->dma_rx_chnum);

        // Here we have to either copy it somewhere else, or notify another thread (queue?)
        // that the data is ready for reading

        if (ht) {
            uint16_t end = (uint16_t) UUSART_RXBUF_LEN / 2;
            UUSART_DMA_HandleRxFromIRQ(unit, end);
            LL_DMA_ClearFlag_HT(priv->dma, priv->dma_rx_chnum);
        }

        if (tc) {
            uint16_t end = (uint16_t) UUSART_RXBUF_LEN;
            UUSART_DMA_HandleRxFromIRQ(unit, end);
            LL_DMA_ClearFlag_TC(priv->dma, priv->dma_rx_chnum);
        }

        if (LL_DMA_IsActiveFlag_TE(isrsnapshot, priv->dma_rx_chnum)) {
            // this shouldn't happen
            dbg("USART DMA TE!");
            LL_DMA_ClearFlag_TE(priv->dma, priv->dma_rx_chnum);
        }
    }
}

/**
 * Start sending a chunk of data.
 * This must be called when the DMA is completed.
 *
 * @param priv
 */
static void UUSART_DMA_TxStart(struct priv *priv)
{
    priv->tx_dma_busy = true;
    assert_param(priv->dma_tx->CNDTR == 0);

    dbg_uusart("DMA_TxStart (nr %d, nw %d)", (int)priv->tx_buf_nr, (int)priv->tx_buf_nw);

    uint16_t nr = priv->tx_buf_nr;
    uint16_t nw = priv->tx_buf_nw;

    if (nr == nw) {
        dbg_uusart("remain=0,do nothing");
        return;
    } // do nothing if we're done

    uint8_t chunk = priv->tx_buffer[nr++];
    //nr += (uint16_t) (4 - (nr & 0b11));
    if (chunk == 0) {
        // wrap-around
        chunk = priv->tx_buffer[0];
        nr = 1;
        assert_param(nr < nw);
    }

    // nr was advanced by the lpad preamble
    priv->tx_buf_nr = nr;
    priv->tx_buf_chunk = chunk; // will be further moved by 'chunk' bytes when dma completes

    dbg_uusart("# TX: chunk start %d, len %d", (int)nr, (int)chunk);
//#if UUSART_DEBUG
//    PUTS(">"); PUTSN((char *) (priv->tx_buffer + nr), chunk); PUTS("<");
//    PUTNL();
//#endif

    LL_DMA_DisableChannel(priv->dma, priv->dma_tx_chnum);
    {
        LL_DMA_ClearFlags(priv->dma, priv->dma_tx_chnum);
        LL_DMA_SetMemoryAddress(priv->dma, priv->dma_tx_chnum, (uint32_t) (priv->tx_buffer + nr));
        LL_DMA_SetDataLength(priv->dma, priv->dma_tx_chnum, chunk);
        LL_USART_ClearFlag_TC(priv->periph);
    }
    LL_DMA_EnableChannel(priv->dma, priv->dma_tx_chnum);
}

COMPILER_ASSERT(UUSART_TXBUF_LEN <= 256); // more would break the "len tag" algorithm

/**
 * Put data on the queue. Only a part may be sent due to a buffer size limit.
 *
 * @param priv
 * @param buffer - buffer to send
 * @param len - buffer size
 * @return number of bytes that were really written (from the beginning)
 */
uint16_t UUSART_DMA_TxQueue(struct priv *priv, const uint8_t *buffer, uint16_t len)
{
    const uint16_t nr = priv->tx_buf_nr;
    uint16_t nw = priv->tx_buf_nw;

    // shortcut for checking a completely full buffer
    if (nw == nr-1 || (nr==0&&nw==UUSART_TXBUF_LEN-1)) {
        dbg_uusart("Buffer full, cant queue");
        return 0;
    }

    dbg_uusart("\r\nQueue..");

    uint16_t used = 0;
    if (nr == nw) {
        used = 0;
    } else if (nw > nr) { // simple linear
        used = (uint16_t) (nw - nr);
    } else if (nw < nr) { // wrapped
        used = (uint16_t) ((UUSART_TXBUF_LEN - nr) + nw);
    }

    dbg_uusart("Trying to send buffer of len %d", (int)len);
    uint16_t avail = (const uint16_t) (UUSART_TXBUF_LEN - 1 - used);
    dbg_uusart("nr %d, nw %d, used %d, avail %d", (int)nr, (int)nw, (int)used, (int)avail);

    // hack to avoid too large chunks (we use 1 byte to store chunk size)
    if (avail > 255) avail = 255;

    uint8_t written = 0;

    // this avoids attempting to write if we don't have space
    if (avail <= 5) {
        dbg_uusart("No space (only %d)", (int) avail);
        return written;
    }

    int cnt = 0;
    while (avail > 0 && written < len) {
        assert_param(cnt < 2); // if more than two, we have a bug and it's repeating infinitely

        cnt++;
        // Padding with chunk information (1 byte: length) - for each chunk
        const uint8_t lpad = 1;

        // Chunk can go max to the end of the buffer
        uint8_t chunk = (uint8_t) MIN((len-written) + lpad, UUSART_TXBUF_LEN - nw);
        if (chunk > avail) chunk = (uint8_t) avail;

        dbg_uusart("nw %d, raw available chunk %d", (int) nw, (int)chunk);
        if (chunk < lpad + 1) {
            // write 0 to indicate a wrap-around
            dbg_uusart("Wrap-around marker at offset %d", (int) nw);
            priv->tx_buffer[nw] = 0;
            nw = 0;
        }
        else {
            // enough space for a preamble + some data
            dbg_uusart("Preamble of %d bytes at offset %d", (int) lpad, (int) nw);
            priv->tx_buffer[nw] = (uint8_t) (chunk - lpad);
            nw += lpad;
            uint8_t datachunk = (uint8_t) (chunk - lpad);
            dbg_uusart("Datachunk len %d at offset %d", (int) datachunk, (int) nw);
//#if UUSART_DEBUG
//            PUTS("mcpy src >"); PUTSN((char *) (buffer), datachunk); PUTS("<\r\n");
//#endif
            memcpy((uint8_t *) (priv->tx_buffer + nw), buffer, datachunk);
//#if UUSART_DEBUG
//            PUTS("mcpy dst >"); PUTSN((char *) (priv->tx_buffer + nw), datachunk); PUTS("<\r\n");
//#endif
            buffer += datachunk;
            nw += datachunk;
            written += datachunk;
            if (nw == UUSART_TXBUF_LEN) nw = 0;
        }
        avail -= chunk;
        dbg_uusart(". end of loop, avail is %d", (int)avail);
    }

    {
        dbg_uusart("Write done -> nr %d, nw %d", (int) nr, (int) nw);

        // FIXME a potential race condition can happen here (but it's unlikely)

        priv->tx_buf_nw = nw;

        if (!priv->tx_dma_busy) {
            dbg_uusart("Write done, requesting DMA.");
            UUSART_DMA_TxStart(priv);
        }
        else {
            dbg_uusart("DMA in progress, not requesting");
        }
    }

    return written;
}

/**
 * Handler for the Tx DMA - completion interrupt
 * @param arg - unit instance
 */
static void UUSART_DMA_TxHandler(void *arg)
{
    Unit *unit = arg;
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    uint32_t isrsnapshot = priv->dma->ISR;
    if (LL_DMA_IsActiveFlag_TC(isrsnapshot, priv->dma_tx_chnum)) {
        // chunk Tx is finished
        dbg_uusart("~ DMA tx done, nr %d, nw %d, chunk %d", (int)priv->tx_buf_nr, (int)priv->tx_buf_nw, (int)priv->tx_buf_chunk);

        priv->tx_buf_nr += priv->tx_buf_chunk;
        if (UUSART_TXBUF_LEN == priv->tx_buf_nr) priv->tx_buf_nr = 0;
        priv->tx_buf_chunk = 0;

        LL_DMA_ClearFlag_TC(priv->dma, priv->dma_tx_chnum);

        // Wait for TC
        while (!LL_USART_IsActiveFlag_TC(priv->periph)); // TODO timeout

        // start the next chunk
        if (priv->tx_buf_nr != priv->tx_buf_nw) {
            dbg_uusart("  Asking for more, if any");
            UUSART_DMA_TxStart(priv);
        } else {
            priv->tx_dma_busy = false;
        }
    }
}


void UUSART_DeInitDMAs(Unit *unit)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    irqd_detach(priv->dma_tx, UUSART_DMA_TxHandler);
    irqd_detach(priv->dma_rx, UUSART_DMA_RxHandler);

    LL_DMA_DeInit(priv->dma, priv->dma_rx_chnum);
    LL_DMA_DeInit(priv->dma, priv->dma_tx_chnum);

    free_ck(priv->rx_buffer);
    free_ck(priv->tx_buffer);
}
