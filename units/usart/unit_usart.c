//
// Created by MightyPork on 2018/01/02.
//

#include "platform.h"
#include "comm/messages.h"
#include "unit_base.h"
#include "unit_usart.h"
#include "tasks/task_msg.h"

#define UUSART_INTERNAL
#include "_internal.h"

static void UUSART_SendReceivedDataToMaster(Job *job)
{
    Unit *unit = job->data1;
    struct priv *priv = unit->data;

    uint32_t readpos = job->d32;
    uint32_t count = job->len;

    // TODO use TF's Multipart sending
    // TODO add API for building reports
    PayloadBuilder pb = pb_start(unit_tmp512, UNIT_TMP_LEN, NULL);
    pb_u8(&pb, unit->callsign);
    pb_u8(&pb, 0x00); // report type "Data received"
    pb_buf(&pb, (uint8_t *) (priv->rx_buffer + readpos), count);
    assert_param(pb.ok);
    com_send_pb(MSG_UNIT_REPORT, &pb);
}

/**
 * Handle received data (we're inside the IRQ)
 *
 * @param unit - handled unit
 * @param endpos - end position in the buffer
 */
void UUSART_DMA_HandleRxFromIRQ(Unit *unit, uint16_t endpos)
{
    assert_param(unit);
    struct priv *priv = unit->data;
    assert_param(priv);

    uint16_t readpos = priv->rx_buf_readpos;
    assert_param(endpos > readpos);

    uint16_t count = (endpos - readpos);

    // We defer it to the job queue
    // FIXME this can starve the shared queue if full duplex is used, we need a second higher priority queue for those report jobs
    Job j = {
        .data1 = unit,
        .d32 = priv->rx_buf_readpos,
        .len = count,
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
