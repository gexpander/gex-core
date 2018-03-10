//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_dac.h"

#define DAC_INTERNAL
#include "_dac_internal.h"

// ------------------------------------------------------------------------

enum DacCmd_ {
    CMD_SET_LEVEL,
};

/** Handle a request message */
static error_t UDAC_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command,
                                  PayloadParser *pp)
{
    switch (command) {
        case CMD_SET_LEVEL: {
            uint16_t ch1 = pp_u16(pp);
            uint16_t ch2 = pp_u16(pp);
            uint32_t dual_reg = DAC->DHR12RD;
            if (ch1 != 0xFFFF) {
                dual_reg = (dual_reg & 0xFFFF0000) | ch1;
            }
            if (ch2 != 0xFFFF) {
                dual_reg = (dual_reg & 0xFFFF) | (ch2<<16);
            }
            DAC->DHR12RD = dual_reg;
            // Trigger a conversion
            DAC->SWTRIGR = (DAC_SWTR_CH1 | DAC_SWTR_CH2);
            return E_SUCCESS;
        }
        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_DAC = {
    .name = "DAC",
    .description = "Analog output",
    // Settings
    .preInit = UDAC_preInit,
    .cfgLoadBinary = UDAC_loadBinary,
    .cfgWriteBinary = UDAC_writeBinary,
    .cfgLoadIni = UDAC_loadIni,
    .cfgWriteIni = UDAC_writeIni,
    // Init
    .init = UDAC_init,
    .deInit = UDAC_deInit,
    // Function
    .handleRequest = UDAC_handleRequest,
};
