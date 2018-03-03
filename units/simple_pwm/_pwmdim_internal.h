//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_PWMDIM_INTERNAL_H
#define GEX_F072_PWMDIM_INTERNAL_H

#ifndef PWMDIM_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Private data structure */
struct priv {
    // settings
    struct {
        uint32_t freq;
        uint8_t ch1_choice;
        uint8_t ch2_choice;
        uint8_t ch3_choice;
        uint8_t ch4_choice;
    } cfg;

    // internal state
    uint32_t freq;
    uint16_t duty1;
    uint16_t duty2;
    uint16_t duty3;
    uint16_t duty4;

    TIM_TypeDef *TIMx;
};

/** Allocate data structure and set defaults */
error_t UPWMDIM_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void UPWMDIM_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void UPWMDIM_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UPWMDIM_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void UPWMDIM_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Finalize unit set-up */
error_t UPWMDIM_init(Unit *unit);

/** Tear down the unit */
void UPWMDIM_deInit(Unit *unit);


error_t UPWMDIM_SetFreq(Unit *unit, uint32_t freq);

error_t UPWMDIM_SetDuty(Unit *unit, uint8_t ch, uint16_t duty1000);

#endif //GEX_F072_PWMDIM_INTERNAL_H
