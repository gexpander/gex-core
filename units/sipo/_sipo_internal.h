//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_SIPO_INTERNAL_H
#define GEX_F072_SIPO_INTERNAL_H

#ifndef SIPO_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Private data structure */
struct priv {
    struct {
        // settings
        Resource pin_store;
        bool store_pol;   //!< Store pulse active edge

        Resource pin_shift;
        bool shift_pol;   //!< Shift clock active edge

        Resource pin_clear;
        bool clear_pol;   //!< Clear signal active level

        char data_pname;
        uint16_t data_pins;
    } cfg;

    // live fields
    uint32_t store_ll;
    uint32_t shift_ll;
    uint32_t clear_ll;

    GPIO_TypeDef *store_port;
    GPIO_TypeDef *shift_port;
    GPIO_TypeDef *clear_port;
    GPIO_TypeDef *data_port;

    uint8_t data_width;
};

/** Allocate data structure and set defaults */
error_t USIPO_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void USIPO_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void USIPO_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t USIPO_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void USIPO_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Finalize unit set-up */
error_t USIPO_init(Unit *unit);

/** Tear down the unit */
void USIPO_deInit(Unit *unit);

// ------------------------------------------------------------------------

/**
 * Write a buffer to the pins.
 * Buffer contains data for the individual channels, sequentially (AAAAAA BBBBBB CCCCCC ...)
 * The bytes are sent LSB first, from the last byte (e.g. 1,2,3 - 3 is sent first, LSB-first).
 *
 * The chunks order is from the lowest to the highest bit
 *
 * @param unit
 * @param buffer - buffer of data to send
 * @param buflen - number of bytes in the buffer
 * @param terminal_data - data to set before sending the store pulse (final data lines state, will not appear in the SIPOs)
 * @return success
 */
error_t UU_SIPO_Write(Unit *unit, const uint8_t *buffer, uint16_t buflen, uint16_t terminal_data);

/**
 * Direct access to the output data pins (may be useful for debugging, or circuits that use them
 * for something else when not loading a new value).
 *
 * @param unit
 * @param data_packed - packed data to set on the output (right-aligned, highest to lowest pin)
 * @return success
 */
error_t UU_SIPO_DirectData(Unit *unit, uint16_t data_packed);

/**
 * Send a clear pulse.
 *
 * @param unit
 * @return success
 */
error_t UU_SIPO_DirectClear(Unit *unit);

/**
 * Send a shift pulse.
 *
 * @param unit
 * @return success
 */
error_t UU_SIPO_DirectShift(Unit *unit);

/**
 * Send a store pulse.
 *
 * @param unit
 * @return success
 */
error_t UU_SIPO_DirectStore(Unit *unit);

#endif //GEX_F072_SIPO_INTERNAL_H
