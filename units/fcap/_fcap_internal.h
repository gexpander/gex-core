//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_FCAP_INTERNAL_H
#define GEX_F072_FCAP_INTERNAL_H

#ifndef FCAP_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

enum fcap_opmode {
    OPMODE_IDLE = 0,
    OPMODE_BUSY = 1, // used after capture is done, before it's reported
    OPMODE_INDIRECT_CONT = 2,
    OPMODE_INDIRECT_BURST = 3, // averaging
    OPMODE_DIRECT_CONT = 4,
    OPMODE_DIRECT_BURST = 5,
    OPMODE_FREE_COUNTER = 6,
    OPMODE_SINGLE_PULSE = 7,
};

/** Private data structure */
struct priv {
    // settings
    struct {
        char signal_pname;   // the input pin - one of TIM2 channels
        uint8_t signal_pnum;

        bool active_level;
        uint8_t direct_presc;
        uint8_t dfilter;
        uint16_t direct_msec;

        enum fcap_opmode startmode;
    } conf;

    // internal state
    TIM_TypeDef *TIMx;
    TIM_TypeDef *TIMy; // used as a timebase source for TIMx in direct mode
    uint32_t ll_ch_b;
    uint32_t ll_ch_a;
    bool a_direct;

    enum fcap_opmode opmode;

    TF_ID request_id;
    uint8_t n_skip; //!< Periods to skip before starting the real capture

    bool active_level; // in PWM mode, the first part that is measured. (if 1, HHHLLL, else LLLHHH). In direct mode, clock polarity
    uint8_t direct_presc;
    uint16_t direct_msec;
    uint8_t dfilter;

    union {
        struct {
            uint32_t ontime; // length of the captured positive pulse in the current interval
            uint32_t last_period; //!< length of the captured interval between two rising edges
            uint32_t last_ontime; //!< length of the last captured ontime
        } ind_cont;

        struct {
            uint32_t ontime; // length of the captured positive pulse in the current interval
            uint64_t period_acu; //!< length of the captured interval between two rising edges, sum
            uint64_t ontime_acu; //!< length of the last captured ontime, sum
            uint16_t n_count; //!< Periods captured
            uint16_t n_target; //!< Periods captured - requested count
        } ind_burst;

        struct {
            uint32_t last_count; //!< Pulse count in the last capture window
        } dir_cont;

        struct {
            uint16_t msec; // capture window length (used in the report callback) - different from the cont time, which is a semi-persistent config
        } dir_burst;
    };
};

/** Allocate data structure and set defaults */
error_t UFCAP_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void UFCAP_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void UFCAP_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UFCAP_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void UFCAP_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Finalize unit set-up */
error_t UFCAP_init(Unit *unit);

/** Tear down the unit */
void UFCAP_deInit(Unit *unit);

// ------------------------------------------------------------------------

void UFCAP_SwitchMode(Unit *unit, enum fcap_opmode opmode);

void UFCAP_TIMxHandler(void *arg);
void UFCAP_TIMyHandler(void *arg);

uint32_t UFCAP_GetFreeCounterValue(Unit *unit);
uint32_t UFCAP_FreeCounterClear(Unit *unit);

#endif //GEX_F072_FCAP_INTERNAL_H
