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
    OPMODE_PWM_CONT = 2,
    OPMODE_PWM_BURST = 3, // averaging
};

/** Private data structure */
struct priv {
    // settings
    char signal_pname;   // the input pin - one of TIM2 channels
    uint8_t signal_pnum;

    // internal state
    TIM_TypeDef *TIMx;
    uint32_t ll_ch_b;
    uint32_t ll_ch_a;
    bool a_direct;

    enum fcap_opmode opmode;

    TF_ID request_id;

    union {
        struct {
            uint32_t ontime; // length of the captured positive pulse in the current interval
            uint32_t last_period; //!< length of the captured interval between two rising edges
            uint32_t last_ontime; //!< length of the last captured ontime
        } pwm_cont;

        struct {
            uint32_t ontime; // length of the captured positive pulse in the current interval
            uint64_t period_acu; //!< length of the captured interval between two rising edges, sum
            uint64_t ontime_acu; //!< length of the last captured ontime, sum
            uint16_t n_count; //!< Periods captured
            uint16_t n_target; //!< Periods captured - requested count
            uint8_t n_skip; //!< Periods to skip before starting the real capture
        } pwm_burst;
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

void UFCAP_TimerHandler(void *arg);

void UFCAP_StopMeasurement(Unit *unit);

void UFCAP_ConfigureForPWMCapture(Unit *unit);

void UFCAP_SwitchMode(Unit *unit, enum fcap_opmode opmode);

#endif //GEX_F072_FCAP_INTERNAL_H
