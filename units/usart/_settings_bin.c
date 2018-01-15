//
// Created by MightyPork on 2018/01/14.
//

#include "platform.h"
#include "unit_base.h"

#define UUSART_INTERNAL
#include "_internal.h"

/** Load from a binary buffer stored in Flash */
void UUSART_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->periph_num = pp_u8(pp);
    priv->remap      = pp_u8(pp);

    priv->baudrate   = pp_u32(pp);
    priv->parity     = pp_u8(pp);
    priv->stopbits   = pp_u8(pp);
    priv->direction  = pp_u8(pp);

    priv->hw_flow_control = pp_u8(pp);
    priv->clock_output    = pp_bool(pp);
    priv->cpol            = pp_bool(pp);
    priv->cpha            = pp_bool(pp);
    priv->lsb_first       = pp_bool(pp);
    priv->width           = pp_u8(pp);

    priv->data_inv = pp_bool(pp);
    priv->rx_inv = pp_bool(pp);
    priv->tx_inv = pp_bool(pp);

    priv->de_output = pp_bool(pp);
    priv->de_polarity = pp_bool(pp);
    priv->de_assert_time = pp_u8(pp);
    priv->de_clear_time = pp_u8(pp);
}

/** Write to a binary buffer for storing in Flash */
void UUSART_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 0); // version

    pb_u8(pb, priv->periph_num);
    pb_u8(pb, priv->remap);

    pb_u32(pb, priv->baudrate);
    pb_u8(pb, priv->parity);
    pb_u8(pb, priv->stopbits);
    pb_u8(pb, priv->direction);

    pb_u8(pb, priv->hw_flow_control);
    pb_bool(pb, priv->clock_output);
    pb_bool(pb, priv->cpol);
    pb_bool(pb, priv->cpha);
    pb_bool(pb, priv->lsb_first);
    pb_u8(pb, priv->width);

    pb_bool(pb, priv->data_inv);
    pb_bool(pb, priv->rx_inv);
    pb_bool(pb, priv->tx_inv);

    pb_bool(pb, priv->de_output);
    pb_bool(pb, priv->de_polarity);
    pb_u8(pb, priv->de_assert_time);
    pb_u8(pb, priv->de_clear_time);
}
