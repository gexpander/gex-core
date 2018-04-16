//
// Created by MightyPork on 2018/02/03.
//
// Defines and prototypes used internally by the ADC unit.
//

#ifndef GEX_F072_ADC_INTERNAL_H
#define GEX_F072_ADC_INTERNAL_H

#ifndef ADC_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

//#define adc_dbg dbg
#define adc_dbg(...) do {} while(0)

#define UADC_MAX_FREQ_FOR_AVERAGING 20000

#define UADC_MAX_CHANNEL 17

enum uadc_opmode {
    ADC_OPMODE_UNINIT, //!< Not yet switched to any mode
    ADC_OPMODE_IDLE,   //!< Idle. Allows immediate value readout and averaging.
    ADC_OPMODE_REARM_PENDING, //!< Idle, waiting for the next sample to re-arm (auto trigger).
    ADC_OPMODE_ARMED,  //!< Armed for a trigger. Direct access and averaging are disabled.
    ADC_OPMODE_TRIGD,  //!< Triggered, sending pre-trigger and streaming captured data.
    ADC_OPMODE_BLCAP,  //!< Capture of fixed length without a trigger
    ADC_OPMODE_STREAM, //!< Unlimited capture
    ADC_OPMODE_EMERGENCY_SHUTDOWN, //!< Used when the buffers overrun to safely transition to IDLE after a delay
};

enum uadc_event {
    EVT_CAPT_START = 50,  //!< Capture start (used in event in the first frame when trigger is detected)
    EVT_CAPT_MORE = 51,   //!< Capture data payload (used as TYPE for all capture types)
    EVT_CAPT_DONE = 52,   //!< End of trig'd or block capture payload (last frame with data),
                          //!<   or a farewell message after closing stream using abort(), in this case without data.
};

/** Private data structure */
struct priv {
    struct {
        uint32_t channels;    //!< bit flags (will be recorded in order 0-15)
        uint8_t sample_time;  //!< 0-7 (corresponds to 1.5-239.5 cycles) - time for the sampling capacitor to charge
        uint32_t frequency;   //!< Timer frequency in Hz. Note: not all frequencies can be achieved accurately
        uint32_t buffer_size; //!< Buffer size in bytes (count 2 bytes per channel per measurement) - faster sampling freq needs bigger buffer
        uint16_t averaging_factor;  //!< Exponential averaging factor 0-1000
        bool enable_averaging;
    } cfg;

    // Peripherals
    ADC_TypeDef *ADCx; //!< The ADC peripheral used
    ADC_Common_TypeDef *ADCx_Common; //!< The ADC common control block
    TIM_TypeDef *TIMx; //!< ADC timing timer instance
    DMA_TypeDef *DMAx; //!< DMA isnatnce used
    uint8_t dma_chnum; //!< DMA channel number
    DMA_Channel_TypeDef *DMA_CHx; //!< DMA channel instance

    uint8_t channel_nums[18];

    // Live config
    float real_frequency;
    uint32_t real_frequency_int;
    uint32_t channels_mask; //!< channels bitfield including tsense and vref
    float avg_factor_as_float;

    uint16_t *dma_buffer; //!< malloc'd buffer for the samples
    uint8_t nb_channels; //!< nbr of enabled adc channels
    uint32_t buf_itemcount;  //!< real size of the buffer in samples (adjusted to fit 2x whole multiple of sample group)

    // Trigger state
    uint32_t trig_stream_remain;   //!< Counter of samples remaining to be sent in the post-trigger stream
    uint16_t trig_holdoff_remain;  //!< Tmp counter for the currently active hold-off
    uint16_t trig_prev_level;      //!< Value of the previous sample, used to detect trigger edge
    uint32_t stream_startpos; //!< Byte offset in the DMA buffer where the next capture for a stream should start.
                              //!<   Updated in TH/TC and on trigger (after the preceding data is sent as a pretrig buffer)

    enum uadc_opmode opmode;  //!< OpMode (state machine state)
    float averaging_bins[18]; //!< Averaging buffers, enough space to accommodate all channels (16 external + 2 internal)
    uint16_t last_samples[18]; //!< If averaging is disabled, the last captured sample is stored here.

    // Trigger config
    uint8_t trigger_source;   //!< number of the pin selected as a trigger source
    uint32_t pretrig_len;     //!< Pre-trigger length, nbr of historical samples to report when trigger occurs
    uint32_t trig_len;        //!< Trigger length, nbr of samples to report AFTER a trigger occurs
    uint16_t trig_level;      //!< Triggering level in LSB
    uint8_t  trig_edge;       //!< Which edge we want to trigger on. 1-rising, 2-falling, 3-both
    bool auto_rearm;          //!< Flag that the trigger should be re-armed after the stream finishes
    uint16_t trig_holdoff;    //!< Trigger hold-off time, set when configuring the trigger
    TF_ID stream_frame_id;    //!< Session ID for multi-part stream (response or report)
    uint8_t stream_serial;    //!< Serial nr of a stream frame

    bool tc_pending;
    bool ht_pending;
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

/** Configure DMA (buffer count etc) */
void UADC_SetupDMA(Unit *unit);

// ------------------------------------------------------------------------

/** DMA half/complete handler */
void UADC_DMA_Handler(void *arg);

/** ADC eod of sequence handler */
void UADC_ADC_EOS_Handler(void *arg);

/** Switch to a different opmode */
void UADC_SwitchMode(Unit *unit, enum uadc_opmode new_mode);

/** Handle trigger - process pre-trigger and start streaming the requested number of samples */
void UADC_HandleTrigger(Unit *unit, uint8_t edge_type, uint64_t timestamp);

/** Handle a periodic tick - expiring the hold-off */
void UADC_updateTick(Unit *unit);

/** Send a end-of-stream message to PC's stream listener so it can shut down. */
void UADC_ReportEndOfStream(Unit *unit);

/** Start a block capture */
void UADC_StartBlockCapture(Unit *unit, uint32_t len, TF_ID frame_id);

/** Start stream */
void UADC_StartStream(Unit *unit, TF_ID frame_id);

/** End stream */
void UADC_StopStream(Unit *unit);

/** Configure frequency */
error_t UADC_SetSampleRate(Unit *unit, uint32_t hertz);

/** Abort capture */
void UADC_AbortCapture(Unit *unit);

#endif //GEX_F072_ADC_INTERNAL_H
