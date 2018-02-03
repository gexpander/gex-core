//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_usart.h"

#define UUSART_INTERNAL
#include "_usart_internal.h"


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
