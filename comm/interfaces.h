//
// Created by MightyPork on 2018/03/23.
//

#ifndef GEX_F072_COM_INTERFACES_H
#define GEX_F072_COM_INTERFACES_H

#include "platform.h"

enum ComportSelection {
    COMPORT_NONE = 0,
    COMPORT_USB = 1,
    COMPORT_USART = 2,
    COMPORT_NORDIC = 3,
    COMPORT_LORA = 4,
};

/**
 * The currently active communication port
 */
extern enum ComportSelection gActiveComport;

/** Switch com transfer if the current one doesnt seem to work */
void com_switch_transfer_if_needed(void);

/** Claim resources that may be needed for alternate transfers */
void com_claim_resources_for_alt_transfers(void);

/** Release resources allocated for alternate transfers */
void com_release_resources_for_alt_transfers(void);

/** Flush the rx buffer */
void com_iface_flush_buffer(void);

#endif //GEX_F072_COM_INTERFACES_H
