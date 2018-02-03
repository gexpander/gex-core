//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_SPI_INTERNAL_H
#define GEX_F072_SPI_INTERNAL_H

#ifndef SPI_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Private data structure */
struct priv {
    uint8_t periph_num; //!< 1 or 2
    uint8_t remap;      //!< SPI remap option

    uint16_t prescaller; //!< Clock prescaller, stored as the dividing factor
    bool cpol;          //!< CPOL setting
    bool cpha;          //!< CPHA setting
    bool tx_only;       //!< If true, Enable only the MOSI line

    bool lsb_first;     //!< Option to send LSB first
    char ssn_port_name; //!< SSN port
    uint16_t ssn_pins;  //!< SSN pin mask

    SPI_TypeDef *periph;
    GPIO_TypeDef *ssn_port;
};

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
void USPI_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void USPI_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t USPI_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void USPI_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
error_t USPI_preInit(Unit *unit);

/** Finalize unit set-up */
error_t USPI_init(Unit *unit);

/** Tear down the unit */
void USPI_deInit(Unit *unit);

#endif //GEX_F072_SPI_INTERNAL_H
