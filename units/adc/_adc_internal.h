//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_ADC_INTERNAL_H
#define GEX_F072_ADC_INTERNAL_H

#ifndef ADC_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

enum uadc_opmode {
    ADC_OPMODE_UNINIT, //!< Not yet switched to any mode
    ADC_OPMODE_IDLE,   //!< Idle, each sample overwrites the previous. Allows immediate value readout and averaging.
    ADC_OPMODE_ARMED,  //!< Armed for a trigger. Direct access and averaging are disabled.
    ADC_OPMODE_TRIGD,  //!< Triggered, sending pre-trigger and streaming captured data.
    ADC_OPMODE_FIXCAPT,//!< Capture of fixed length without a trigger
    ADC_OPMODE_STREAM, //!< Unlimited capture
};

/** Private data structure */
struct priv {
    // settings
    uint16_t channels;    //!< bit flags (will be recorded in order 0-15)
    bool enable_tsense;   //!< append a signal from the temperature channel (voltage proportional to Tj)
    bool enable_vref;     //!< append a signal from the internal voltage reference
    uint8_t sample_time;  //!< 0-7 (corresponds to 1.5-239.5 cycles) - time for the sampling capacitor to charge
    uint32_t frequency;   //!< Timer frequency in Hz. Note: not all frequencies can be achieved accurately
    uint16_t buffer_size; //!< Buffer size in bytes (count 2 bytes per channel per measurement) - faster sampling freq needs bigger buffer

    bool enable_averaging;   //!< Enable exponential averaging
    uint16_t averaging_factor;  //!< Exponential averaging factor 0-1000

    // internal state
    uint32_t extended_channels_mask; //!< channels bitfield including tsense and vref
    float avg_factor_as_float;
    ADC_TypeDef *ADCx; //!< The ADC peripheral used
    ADC_Common_TypeDef *ADCx_Common; //!< The ADC common control block
    TIM_TypeDef *TIMx; //!< ADC timing timer instance
    DMA_TypeDef *DMAx; //!< DMA isnatnce used
    uint8_t dma_chnum; //!< DMA channel number
    DMA_Channel_TypeDef *DMA_CHx; //!< DMA channel instance
    uint16_t *dma_buffer;     //!< malloc'd buffer for the samples
    uint8_t nb_channels;      //!< nbr of enabled adc channels
    uint16_t dma_buffer_itemcount; //!< real size of the buffer (adjusted from the configured size to evenly encompass 2*size of one sample)

    enum uadc_opmode opmode;  //!< OpMode (state machine state)
    union {
        float averaging_bins[18]; //!< Averaging buffers, enough space to accommodate all channels (16 external + 2 internal)
        uint16_t last_sample[18]; //!< If averaging is disabled, the last captured sample is stored here.
    };
    uint8_t trigger_source;   //!< number of the pin selected as a trigger source
    uint16_t pretrig_len;     //!< Pre-trigger length, nbr of historical samples to report when trigger occurs
    uint32_t trig_len;        //!< Trigger length, nbr of samples to report AFTER a trigger occurs
    uint16_t trig_level;      //!< Triggering level in LSB
    uint16_t trig_prev_level;      //!< Value of the previous sample, used to detect trigger edge
    uint8_t  trig_edge;       //!< Which edge we want to trigger on. 1-rising, 2-falling, 3-both
    uint32_t trig_stream_remain;   //!< Counter of samples remaining to be sent in the post-trigger stream
    bool auto_rearm;          //!< Flag that the trigger should be re-armed after the stream finishes
    uint16_t trig_holdoff;    //!< Trigger hold-off time, set when configuring the trigger
    uint16_t trig_holdoff_remain; //!< Tmp counter for the currently active hold-off
    uint16_t stream_startpos; //!< Byte offset in the DMA buffer where the next capture for a stream should start.
                              //!<   Updated in TH/TC and on trigger (after the preceding data is sent as a pretrig buffer)
};

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

// ------------------------------------------------------------------------

/** DMA half/complete handler */
void UADC_DMA_Handler(void *arg);

/** ADC eod of sequence handler */
void UADC_ADC_EOS_Handler(void *arg);

/** Switch to a different opmode */
void UADC_SwitchMode(Unit *unit, enum uadc_opmode new_mode);

/** Handle trigger - process pre-trigger and start streaming the requested number of samples */
void UADC_HandleTrigger(Unit *unit, bool rising, uint64_t timestamp);

/** Handle a periodic tick - expiring the hold-off */
void UADC_updateTick(Unit *unit);

#endif //GEX_F072_ADC_INTERNAL_H
