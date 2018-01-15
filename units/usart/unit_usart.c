//
// Created by MightyPork on 2018/01/02.
//

#include "platform.h"
#include "comm/messages.h"
#include "unit_base.h"
#include "unit_usart.h"

#define UUSART_INTERNAL
#include "_internal.h"

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
    uint8_t *start = (uint8_t *) (priv->rx_buffer + readpos);

    // Do something with the data...
    PUTSN((char *) start, count);
    PUTNL();

    // Move the read cursor, wrap around if needed
    if (endpos == UUSART_RXBUF_LEN) endpos = 0;
    priv->rx_buf_readpos = endpos;
}


#if 0
static error_t usart_wait_until_flag(struct priv *priv, uint32_t flag, bool stop_state)
{
    uint32_t t_start = HAL_GetTick();
    while (((priv->periph->ISR & flag) != 0) != stop_state) {
        if (HAL_GetTick() - t_start > 10) {
            return E_HW_TIMEOUT;
        }
    }
    return E_SUCCESS;
}

static error_t sync_send(struct priv *priv, const uint8_t *buf, uint32_t len)
{
    while (len > 0) {
        TRY(usart_wait_until_flag(priv, USART_ISR_TXE, 1));
        priv->periph->TDR = *buf++;
        len--;
    }
    TRY(usart_wait_until_flag(priv, USART_ISR_TC, 1));
    return E_SUCCESS;
}
#endif


enum PinCmd_ {
    CMD_WRITE = 0,
};

/** Handle a request message */
static error_t UUSART_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    switch (command) {
        case CMD_WRITE:;
            uint32_t len;
            const uint8_t *pld = pp_tail(pp, &len);

            uint32_t t_start = HAL_GetTick();
            while (len > 0) {
                // this should be long enough even for the slowest bitrates and 512 bytes
                if (HAL_GetTick() - t_start > 5000) {
                    return E_HW_TIMEOUT;
                }

                uint16_t chunk = UUSART_DMA_TxQueue(priv, pld, (uint16_t) len);

                pld += chunk;
                len -= chunk;

                // We give up control if there's another thread waiting and this isn't the last cycle
                if (len > 0) {
                    osThreadYield();
                }
            }

            return E_SUCCESS;
            //return E_NOT_IMPLEMENTED;

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
    .handleRequest = UUSART_handleRequest,
};
