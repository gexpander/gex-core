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
    NOISE_NONE = 0b00, // 0
    NOISE_WHITE = 0b01, // 1
    NOISE_TRIANGLE = 0b10, // 2
};

enum UDAC_Waveform {
    UDAC_WAVE_DC,
    UDAC_WAVE_SINE,
    UDAC_WAVE_TRIANGLE,
    UDAC_WAVE_SAWTOOTH_UP,
    UDAC_WAVE_SAWTOOTH_DOWN,
    UDAC_WAVE_RECTANGLE,
};

struct udac_channel_cfg {
    bool enable;
    bool buffered;
    enum UDAC_Noise noise_type;
    uint8_t noise_level; // 0-11
};

// 0 - 1 MHz, 2-500k, 4-250k, 8-125k
#define UDAC_TIM_FREQ_DIVIDER 8

#define UDAC_INDEX_WIDTH 13 // corresponds to 8192 places
#define UDAC_INDEX_SHIFT (32 - UDAC_INDEX_WIDTH)
#define UDAC_MAX_INDEX   ((1 << UDAC_INDEX_WIDTH) - 1)
#define UDAC_VALUE_COUNT (1 << UDAC_INDEX_WIDTH)

extern const uint8_t LUT_sine_8192_quad_packed[];

struct udac_channel_live {
    enum UDAC_Noise noise_type;
    uint8_t noise_level; // 0-11
    enum UDAC_Waveform waveform;

    uint16_t rectangle_ontime; // for rectangle wave, 0-8191
    uint16_t rectangle_high;
    uint16_t rectangle_low;
    uint16_t dc_level; // for DC wave

    uint32_t counter;
    uint32_t increment;

    // last set phase if the frequencies are the same
    // - can be used for live frequency changes without reset (meaningful only with matching increment values)
    uint16_t phase;
    uint16_t last_index;
    uint16_t last_value;
};

/** Private data structure */
struct priv {
    // settings
    struct {
        struct udac_channel_cfg ch[2];
    } cfg;

    // internal state
    struct udac_channel_live ch[2];
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

void UDAC_HandleIT(void *arg);

error_t UDAC_SetFreq(Unit *unit, int channel, float freq);

void UDAC_ToggleTimerIfNeeded(Unit *unit);

#endif //GEX_F072_DAC_INTERNAL_H
