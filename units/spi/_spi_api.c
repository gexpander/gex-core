//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"
#include "unit_spi.h"

#define SPI_INTERNAL
#include "_spi_internal.h"

static error_t spi_wait_until_flag(struct priv *priv, uint32_t flag, bool stop_state)
{
    uint32_t t_start = HAL_GetTick();
    while (((priv->periph->SR & flag) != 0) != stop_state) {
        if (HAL_GetTick() - t_start > 10) {
            return E_HW_TIMEOUT;
        }
    }
    return E_SUCCESS;
}

/**
 * Perform a low level SPI transfer
 *
 * @param priv - private object of the SPI unit
 * @param request - request buffer
 * @param response - response buffer
 * @param req_len - request len
 * @param resp_skip - response skip bytes
 * @param resp_len - response len
 * @return success
 */
static error_t xfer_do(struct priv *priv, const uint8_t *request,
                       uint8_t *response,
                       uint32_t req_len,
                       uint32_t resp_skip,
                       uint32_t resp_len)
{
    // TODO this is slow, use DMA

    if (response == NULL) resp_len = 0;

    // avoid skip causing stretch beyond tx window if nothing is to be read back
    if (resp_len == 0) resp_skip = 0;

    // in tx only mode, return zeros
    if (priv->tx_only && resp_len>0) {
        memset(response, 0, resp_len);
    }

    uint8_t tb;
    uint32_t end = MAX(req_len, resp_len + resp_skip);
    for (uint32_t i = 0; i < end; i++) {
        if (i < req_len) tb = *request++;
        else tb = 0;

        TRY(spi_wait_until_flag(priv, SPI_SR_TXE, true));
        LL_SPI_TransmitData8(priv->periph, tb);

        if (!priv->tx_only) {
            TRY(spi_wait_until_flag(priv, SPI_SR_RXNE, true));
            uint8_t rb = LL_SPI_ReceiveData8(priv->periph);

            if (resp_skip > 0) resp_skip--;
            else if (resp_len > 0) {
                resp_len--;
                *response++ = rb;
            }
        }
    }
    return E_SUCCESS;
}

error_t UU_SPI_Multicast(Unit *unit, uint16_t slaves,
                         const uint8_t *request, uint32_t req_len)
{
    struct priv *priv= unit->data;
    uint16_t mask = pinmask_spread(slaves, priv->ssn_pins);
    priv->ssn_port->BRR = mask;
    {
        TRY(xfer_do(priv, request, NULL, req_len, 0, 0));
    }
    priv->ssn_port->BSRR = mask;

    return E_SUCCESS;
}

error_t UU_SPI_Write(Unit *unit, uint8_t slave_num,
                     const uint8_t *request, uint8_t *response,
                     uint32_t req_len, uint32_t resp_skip, uint32_t resp_len)
{
    struct priv *priv= unit->data;

    uint16_t mask = pinmask_spread((uint16_t) (1 << slave_num), priv->ssn_pins);
    priv->ssn_port->BRR = mask;
    {
        TRY(xfer_do(priv, request, response, req_len, resp_skip, resp_len));
    }
    priv->ssn_port->BSRR = mask;

    return E_SUCCESS;
}
