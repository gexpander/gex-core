//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_DAC_INTERNAL_H
#define GEX_F072_DAC_INTERNAL_H

#ifndef DAC_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

enum UDAC_Noise {
    NOISE_NONE = 0b00,
    NOISE_WHITE = 0b01,
    NOISE_TRIANGLE = 0b10,
};

struct udac_channel_cfg {
    bool enable;
    bool buffered;
    enum UDAC_Noise noise_type;
    uint8_t noise_level; // 0-11
};

struct udac_channel_live {
    enum UDAC_Noise noise_type;
    uint8_t noise_level; // 0-11
};

/** Private data structure */
struct priv {
    // settings
    struct {
        struct udac_channel_cfg ch1;
        struct udac_channel_cfg ch2;
    } cfg;

    // internal state
    struct udac_channel_live ch1;
    struct udac_channel_live ch2;
    TIM_TypeDef *TIMx; // timer used for the DDS function
};

/** Allocate data structure and set defaults */
error_t UDAC_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void UDAC_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void UDAC_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UDAC_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void UDAC_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Finalize unit set-up */
error_t UDAC_init(Unit *unit);

/** Tear down the unit */
void UDAC_deInit(Unit *unit);

void UDAC_Reconfigure(Unit *unit);

#endif //GEX_F072_DAC_INTERNAL_H
