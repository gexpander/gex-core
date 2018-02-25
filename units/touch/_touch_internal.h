//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_TOUCH_INTERNAL_H
#define GEX_F072_TOUCH_INTERNAL_H

#ifndef TOUCH_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

#define TSC_DEBUG 0

#if TSC_DEBUG
#define dbg_touch(...) dbg(...)
#else
#define dbg_touch(...) do{}while(0)
#endif

/** Private data structure */
struct priv {
    // settings
    struct {
        uint8_t charge_time;  // 1-16 -> 0..15
        uint8_t drain_time;   // 1-16 -> 0..15
        uint8_t spread_deviation; // 1-128, 0=off ... 0-127, 0 sets 0 to SSE
        uint8_t ss_presc;     // 1-2 -> 0..1
        uint8_t pg_presc;     // 1,2,4,8,16,32,64,128 -> 0..7 when writing to the periph
        uint8_t sense_timeout; // 1-7 -> 0..6 hex when writing to the periph
        // the schmitts must be disabled on all used channels, restored to 0xFFFF on deinit
        uint8_t group_scaps[8];
        uint8_t group_channels[8];
    } cfg;

    uint8_t next_phase;
    uint32_t channels_phase[3];
    uint8_t pgen_phase[3];
    uint16_t readouts[32];
};

extern const char *utouch_group_labels[8];
extern const Resource utouch_group_rscs[8][4];

/** Allocate data structure and set defaults */
error_t UTOUCH_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void UTOUCH_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void UTOUCH_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UTOUCH_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void UTOUCH_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Finalize unit set-up */
error_t UTOUCH_init(Unit *unit);

/** Tear down the unit */
void UTOUCH_deInit(Unit *unit);

#endif //GEX_F072_TOUCH_INTERNAL_H
