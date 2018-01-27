//
// Created by MightyPork on 2018/01/02.
//

#include "unit_base.h"
#include "unit_usart.h"

#define UUSART_INTERNAL
#include "_internal.h"

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
 * Handle received data (we're inside the IRQ)
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
    scheduleJob(&j);

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


error_t UU_USART_Write(Unit *unit, const uint8_t *buffer, uint32_t len)
{
    CHECK_TYPE(unit, &UNIT_USART);
    struct priv *priv = unit->data;

    uint32_t t_start = HAL_GetTick();
    while (len > 0) {
        // this should be long enough even for the slowest bitrates and 512 bytes
        if (HAL_GetTick() - t_start > 5000) {
            return E_HW_TIMEOUT;
        }

        uint16_t chunk = UUSART_DMA_TxQueue(priv, buffer, (uint16_t) len);

        buffer += chunk;
        len -= chunk;

        // We give up control if there's another thread waiting and this isn't the last cycle
        if (len > 0) {
            osThreadYield();
        }
    }

    return E_SUCCESS;
}

error_t UU_USART_WriteSync(Unit *unit, const uint8_t *buffer, uint32_t len)
{
    CHECK_TYPE(unit, &UNIT_USART);
    struct priv *priv = unit->data;

    TRY(UU_USART_Write(unit, buffer, len));

    // Now wait for the last DMA to complete
    uint32_t t_start = HAL_GetTick();
    while (priv->tx_dma_busy) {
        if (HAL_GetTick() - t_start > 1000) {
            return E_HW_TIMEOUT;
        }
    }

    return E_SUCCESS;
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
        case CMD_WRITE:
            pld = pp_tail(pp, &len);
            TRY(UU_USART_Write(unit, pld, len));
            return E_SUCCESS;

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
