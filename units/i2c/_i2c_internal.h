//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_I2C_INTERNAL_H
#define GEX_F072_I2C_INTERNAL_H

#ifndef I2C_INTERNAL
#error bad include!
#endif

/** Private data structure */
struct priv {
    uint8_t periph_num; //!< 1 or 2
    uint8_t remap;      //!< I2C remap option

    bool anf;    //!< Enable analog noise filter
    uint8_t dnf; //!< Enable digital noise filter (1-15 ... max spike width)
    uint8_t speed; //!< 0 - Standard, 1 - Fast, 2 - Fast+

    I2C_TypeDef *periph;
};

/** Load from a binary buffer stored in Flash */
void UI2C_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void UI2C_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UI2C_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void UI2C_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
error_t UI2C_preInit(Unit *unit);

/** Finalize unit set-up */
error_t UI2C_init(Unit *unit);

/** Tear down the unit */
void UI2C_deInit(Unit *unit);

// ------------------------------------------------------------------------

#endif //GEX_F072_I2C_INTERNAL_H
