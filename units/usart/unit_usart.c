//
// Created by MightyPork on 2018/01/02.
//

#include "platform.h"
#include "comm/messages.h"
#include "unit_base.h"
#include "unit_usart.h"

#define UUSART_INTERNAL
#include "_internal.h"

/** Allocate data structure and set defaults */
extern error_t UUSART_preInit(Unit *unit);
// ------------------------------------------------------------------------
/** Load from a binary buffer stored in Flash */
extern void UUSART_loadBinary(Unit *unit, PayloadParser *pp);
/** Write to a binary buffer for storing in Flash */
extern void UUSART_writeBinary(Unit *unit, PayloadBuilder *pb);
// ------------------------------------------------------------------------
/** Parse a key-value pair from the INI file */
extern error_t UUSART_loadIni(Unit *unit, const char *key, const char *value);
/** Generate INI file section for the unit */
extern void UUSART_writeIni(Unit *unit, IniWriter *iw);
// ------------------------------------------------------------------------
/** Tear down the unit */
extern void UUSART_deInit(Unit *unit);
/** Finalize unit set-up */
extern error_t UUSART_init(Unit *unit);
// ------------------------------------------------------------------------

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

            TRY(sync_send(priv, pld, len));
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
