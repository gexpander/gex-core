//
// Created by MightyPork on 2018/01/02.
//

#include "unit_base.h"
#include "unit_usart.h"

#define UUSART_INTERNAL
#include "_usart_internal.h"

/**
 * Data RX handler, run on the jobs thread.
 *
 * unit - unit
 * timestamp - timestamp
 * data1 - read start position
 * data2 - nr of bytes to read
 *
 * @param job
 */
static void UUSART_SendReceivedDataToMaster(Job *job)
{
    Unit *unit = job->unit;
    struct priv *priv = unit->data;

    uint16_t readpos = (uint16_t) job->data1;
    uint16_t count = (uint16_t) job->data2;

    EventReport event = {
        .unit = job->unit,
        .timestamp = job->timestamp,
        .data = (uint8_t *) (priv->rx_buffer + readpos),
        .length = count,
    };

    EventReport_Send(&event);
}

/**
 * Handle received data (we're inside the IRQ).
 * This is called either from a DMA complete / half interrupot,
 * or form the timeout interrupt.
 *
 * @param unit - handled unit
 * @param endpos - end position in the buffer
 */
void UUSART_DMA_HandleRxFromIRQ(Unit *unit, uint16_t endpos)
{
    const uint64_t ts = PTIM_GetMicrotime();

    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    uint16_t readpos = priv->rx_buf_readpos;
    assert_param(endpos > readpos);

    uint16_t count = (endpos - readpos);

    // We defer it to the job queue
    Job j = {
        .unit = unit,
        .timestamp = ts,
        .data1 = priv->rx_buf_readpos,
        .data2 = count,
        .cb = UUSART_SendReceivedDataToMaster
    };
    scheduleJob(&j); // TODO disable unit on failure

    // Move the read cursor, wrap around if needed
    if (endpos == UUSART_RXBUF_LEN) endpos = 0;
    priv->rx_buf_readpos = endpos;
}

/**
 * Timed tick (ISR) - check timeout
 */
void UUSART_Tick(Unit *unit)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    if (priv->rx_last_dmapos == priv->dma_rx->CNDTR) {
        uint16_t endpos = (uint16_t) (UUSART_RXBUF_LEN - priv->rx_last_dmapos);
        if (endpos != priv->rx_buf_readpos) {
            UUSART_DMA_HandleRxFromIRQ(unit, endpos);
        }
    } else {
        priv->rx_last_dmapos = (uint16_t) priv->dma_rx->CNDTR;
    }
}

enum PinCmd_ {
    CMD_WRITE = 0,
    CMD_WRITE_SYNC = 1,
};

/** Handle a request message */
static error_t UUSART_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    uint32_t len;
    const uint8_t *pld;
    switch (command) {
        /**
         * Write bytes to the USART, without waiting for completion.
         * May wait until there is space in the DMA buffer.
         */
        case CMD_WRITE:
            pld = pp_tail(pp, &len);
            TRY(UU_USART_Write(unit, pld, len));
            return E_SUCCESS;

        /**
         * Write bytes to the USART. Payload consists of the data to send.
         * Waits for completion.
         */
        case CMD_WRITE_SYNC:
            pld = pp_tail(pp, &len);
            TRY(UU_USART_WriteSync(unit, pld, len));
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_USART = {
    .name = "USART",
    .description = "Serial port",
    // Settings
    .preInit = UUSART_preInit,
    .cfgLoadBinary = UUSART_loadBinary,
    .cfgWriteBinary = UUSART_writeBinary,
    .cfgLoadIni = UUSART_loadIni,
    .cfgWriteIni = UUSART_writeIni,
    // Init
    .init = UUSART_init,
    .deInit = UUSART_deInit,
    // Function
    .updateTick = UUSART_Tick,
    .handleRequest = UUSART_handleRequest,
};
