//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_dac.h"

#define DAC_INTERNAL
#include "_dac_internal.h"

// ------------------------------------------------------------------------

// Works OK up to about 20 kHz, could work faster with a faster interrupt
// (may be possible with some optimizations / adjusting priorities...)

enum DacCmd_ {
    CMD_WAVE_DC = 0,
    CMD_WAVE_SINE = 1,
    CMD_WAVE_TRIANGLE = 2,
    CMD_WAVE_SAWTOOTH_UP = 3,
    CMD_WAVE_SAWTOOTH_DOWN = 4,
    CMD_WAVE_RECTANGLE = 5,

    CMD_SYNC = 10,

    CMD_SET_FREQUENCY = 20,
    CMD_SET_PHASE = 21,
    CMD_SET_DITHER = 22,
};

/** Handle a request message */
static error_t UDAC_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    // Exceptions that aren't per-channel
    switch (command) {
        case CMD_SYNC:
            dbg("Sync");
            priv->ch[0].counter = priv->ch[0].phase << UDAC_INDEX_SHIFT;
            priv->ch[1].counter = priv->ch[1].phase << UDAC_INDEX_SHIFT;
            return E_SUCCESS;
    }

    uint8_t channels = pp_u8(pp);

    bool want_reinit = false;
    bool want_tog_timer = false;

    // TODO move this stuff to the api file

    for (int i = 0; i < 2; i++) {
        if (channels & (1<<i)) {
            switch (command) {
                case CMD_SET_FREQUENCY:;
                    float freq = pp_float(pp);
                    TRY(UDAC_SetFreq(unit, i, freq));
                    break;

                case CMD_WAVE_DC:
                    priv->ch[i].dc_level = pp_u16(pp);
                    priv->ch[i].waveform = UDAC_WAVE_DC;
                    want_tog_timer = true;
                    break;

                case CMD_WAVE_SINE:
                    priv->ch[i].waveform = UDAC_WAVE_SINE;
                    want_tog_timer = true;
                    break;

                case CMD_WAVE_TRIANGLE:
                    priv->ch[i].waveform = UDAC_WAVE_TRIANGLE;
                    want_tog_timer = true;
                    break;

                case CMD_WAVE_SAWTOOTH_UP:
                    priv->ch[i].waveform = UDAC_WAVE_SAWTOOTH_UP;
                    want_tog_timer = true;
                    break;

                case CMD_WAVE_SAWTOOTH_DOWN:
                    priv->ch[i].waveform = UDAC_WAVE_SAWTOOTH_DOWN;
                    want_tog_timer = true;
                    break;

                case CMD_WAVE_RECTANGLE:;
                    uint16_t ontime = pp_u16(pp);
                    uint16_t high = pp_u16(pp);
                    uint16_t low = pp_u16(pp);

                    // use 0xFFFF to skip setting the value
                    if (high < 4096) priv->ch[i].rectangle_high = high;
                    if (low < 4096) priv->ch[i].rectangle_low = low;
                    if (ontime <= UDAC_VALUE_COUNT) priv->ch[i].rectangle_ontime = ontime;

//                    dbg("Set rect hi %d, low %d", (int)priv->ch[i].rectangle_high, (int)priv->ch[i].rectangle_low);

                    priv->ch[i].waveform = UDAC_WAVE_RECTANGLE;
                    want_tog_timer = true;
                    break;

                case CMD_SET_PHASE:;
                    uint16_t ph = pp_u16(pp);
                    uint32_t newphase = ph << UDAC_INDEX_SHIFT;
                    uint32_t oldphase = priv->ch[i].phase << UDAC_INDEX_SHIFT;
                    int32_t diff = newphase - oldphase;
                    priv->ch[i].counter += diff;
                    priv->ch[i].phase = ph;
                    break;

                case CMD_SET_DITHER:;
                    uint8_t noisetype = pp_u8(pp); // 0-none, 1-random, 2-triangle
                    uint8_t noisebits = pp_u8(pp);

                    // type 0xFF = not set
                    if (noisetype <= 2) {
                        priv->ch[i].noise_type = (enum UDAC_Noise) noisetype;
                    }

                    // bits 0xFF = not set
                    if (noisebits >= 1 && noisebits <= 12) {
                        priv->ch[i].noise_level = (uint8_t) (noisebits - 1);
                    }

//                    dbg("Ch %d: Dither type %d, level %d", i,
//                        (int)priv->ch[i].noise_type,
//                        (int)priv->ch[i].noise_level);

                    want_reinit = true;
                    break;

                default:
                    return E_UNKNOWN_COMMAND;
            }
        }
    }

    if (want_reinit) {
        UDAC_Reconfigure(unit);
    }

    if (want_tog_timer) {
        UDAC_ToggleTimerIfNeeded(unit);
    }

    return E_SUCCESS;
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_DAC = {
    .name = "DAC",
    .description = "Two-channel analog output with waveforms",
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
