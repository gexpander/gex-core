//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_ADC_INTERNAL_H
#define GEX_F072_ADC_INTERNAL_H

#ifndef ADC_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/** Private data structure */
struct priv {
    // settings
    uint16_t channels;    //!< bit flags (will be recorded in order 0-15)
    bool enable_tsense;   //!< append a signal from the temperature channel (voltage proportional to Tj)
    bool enable_vref;  //!< append a signal from the internal voltage reference
    uint8_t sample_time;  //!< 0-7 (corresponds to 1.5-239.5 cycles) - time for the sampling capacitor to charge
    uint32_t frequency;   //!< Timer frequency in Hz. Note: not all frequencies can be achieved accurately

    // TODO averaging (maybe a separate component?)
    // TODO threshold watchdog with hysteresis (maybe a separate component?)
    // TODO trigger level, edge direction, hold-off, pre-trigger buffer (extract from the DMA buffer)

    // internal state
    ADC_TypeDef *ADCx;
    ADC_Common_TypeDef *ADCx_Common;
    TIM_TypeDef *TIMx;
    DMA_TypeDef *DMAx;
    uint8_t dma_chnum;
    DMA_Channel_TypeDef *DMA_CHx;
    uint16_t *dma_buffer;
    uint8_t nb_channels; // nr of enabled adc channels
    uint16_t dma_buffer_size; // real number of bytes
};

// max size of the DMA buffer. The actual buffer size will be adjusted to accommodate
// an even number of sample groups (sets of channels)
#define UADC_DMA_MAX_BUF_LEN 512

/** Allocate data structure and set defaults */
error_t UADC_preInit(Unit *unit);

/** Load from a binary buffer stored in Flash */
void UADC_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void UADC_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UADC_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void UADC_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Finalize unit set-up */
error_t UADC_init(Unit *unit);

/** Tear down the unit */
void UADC_deInit(Unit *unit);

#endif //GEX_F072_ADC_INTERNAL_H
