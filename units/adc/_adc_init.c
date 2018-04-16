//
// Created by MightyPork on 2018/02/03.
//
// ADC unit init and de-init functions
//

#include "platform.h"
#include "unit_base.h"

#define ADC_INTERNAL
#include "_adc_internal.h"

/** Allocate data structure and set defaults */
error_t UADC_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    priv->cfg.channels = 1<<16; // Tsense by default - always available, easy testing
    priv->cfg.sample_time = 0b010; // 13.5c - good enough and the default 0b00 value really is useless
    priv->cfg.frequency = 1000;
    priv->cfg.buffer_size = 256; // in half-words
    priv->cfg.averaging_factor = 500; // 0.5
    priv->cfg.enable_averaging = true;

    priv->opmode = ADC_OPMODE_UNINIT;

    return E_SUCCESS;
}


/** Configure frequency */
error_t UADC_SetSampleRate(Unit *unit, uint32_t hertz)
{
    struct priv *priv = unit->data;

    uint16_t presc;
    uint32_t count;
    if (!hw_solve_timer(PLAT_APB1_HZ, hertz, true, &presc, &count, &priv->real_frequency)) {
        dbg("Failed to resolve timer params.");
        return E_BAD_VALUE;
    }
    adc_dbg("Frequency error %d ppm, presc %d, count %d",
        (int) lrintf(1000000.0f *  ((priv->real_frequency - hertz) / (float) hertz)),
        (int) presc, (int) count);

    LL_TIM_SetPrescaler(priv->TIMx, (uint32_t) (presc - 1));
    LL_TIM_SetAutoReload(priv->TIMx, count - 1);

    priv->real_frequency_int = hertz;

    return E_SUCCESS;
}

/**
 * Set up the ADC DMA.
 * This is split to its own function because it's also called when the user adjusts the
 * enabled channels and we need to re-configure it.
 *
 * @param unit
 */
void UADC_SetupDMA(Unit *unit)
{
    struct priv *priv = unit->data;

    adc_dbg("Setting up DMA");
    {
        uint32_t itemcount = priv->nb_channels * (priv->cfg.buffer_size / (priv->nb_channels));
        if (itemcount % 2 == 1) itemcount -= priv->nb_channels; // ensure the count is even
        priv->buf_itemcount = itemcount;

        adc_dbg("DMA item count is %d (%d bytes), There are %d samples per group.",
                (int)priv->buf_itemcount,
                (int)(priv->buf_itemcount * sizeof(uint16_t)),
                (int)priv->nb_channels);

        {
            LL_DMA_InitTypeDef init;
            LL_DMA_StructInit(&init);
            init.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;

            init.Mode = LL_DMA_MODE_CIRCULAR;
            init.NbData = itemcount;

            init.PeriphOrM2MSrcAddress = (uint32_t) &priv->ADCx->DR;
            init.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_HALFWORD;
            init.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;

            init.MemoryOrM2MDstAddress = (uint32_t) priv->dma_buffer;
            init.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_HALFWORD;
            init.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;

            assert_param(SUCCESS == LL_DMA_Init(priv->DMAx, priv->dma_chnum, &init));
        }

//        LL_DMA_EnableChannel(priv->DMAx, priv->dma_chnum); // this is done in the switch mode func now
    }
}

/** Finalize unit set-up */
error_t UADC_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    // Written for F072 which has only one ADC

    TRY(rsc_claim(unit, R_ADC1));
    TRY(rsc_claim(unit, R_DMA1_1));
    TRY(rsc_claim(unit, R_TIM15));

    priv->DMAx = DMA1;
    priv->DMA_CHx = DMA1_Channel1;
    priv->dma_chnum = 1;
    priv->ADCx = ADC1;
    priv->ADCx_Common = ADC1_COMMON;
    priv->TIMx = TIM15;

    // ----------------------- CONFIGURE PINS --------------------------
    {
        // Claim and configure all analog pins
        priv->nb_channels = 0;
        for (uint8_t i = 0; i <= UADC_MAX_CHANNEL; i++) {
            if (priv->cfg.channels & (1UL << i)) {
                priv->channel_nums[priv->nb_channels] = (uint8_t) i;
                priv->nb_channels++;

                do {
                    char c;
                    uint8_t num;
                    if (i <= 7) {
                        c = 'A';
                        num = i;
                    }
                    else if (i <= 9) {
                        c = 'B';
                        num = (uint8_t) (i - 8);
                    }
                    else if (i <= 15) {
                        c = 'C';
                        num = (uint8_t) (i - 10);
                    }
                    else {
                        break;
                    }

                    TRY(rsc_claim_pin(unit, c, num));
                    uint32_t ll_pin = hw_pin2ll(num, &suc);
                    GPIO_TypeDef *port = hw_port2periph(c, &suc);
                    assert_param(suc);

                    LL_GPIO_SetPinPull(port, ll_pin, LL_GPIO_PULL_NO);
                    LL_GPIO_SetPinMode(port, ll_pin, LL_GPIO_MODE_ANALOG);
                } while (0);
            }
        }

        if (priv->nb_channels == 0) {
            dbg("Need at least 1 channel");
            return E_BAD_CONFIG;
        }

        // ensure some minimal space is available
        if (priv->cfg.buffer_size < priv->nb_channels * 2) {
            dbg("Insufficient buf size");
            return E_BAD_CONFIG;
        }
    }

    // ---------------- Alloc the buffer ----------------------
    adc_dbg("Allocating buffer of size %d half-words", (int)priv->cfg.buffer_size);
    priv->dma_buffer = calloc_ck(priv->cfg.buffer_size, sizeof(uint16_t));
    if (NULL == priv->dma_buffer) return E_OUT_OF_MEM;
    assert_param(((uint32_t) priv->dma_buffer & 3) == 0); // must be aligned

    // ------------------- ENABLE CLOCKS --------------------------
    {
        // enable peripherals clock
        hw_periph_clock_enable(priv->ADCx);
        hw_periph_clock_enable(priv->TIMx);
        // DMA and GPIO clocks are enabled on startup automatically
    }

    // ------------------- CONFIGURE THE TIMER --------------------------
    adc_dbg("Setting up TIMER");
    {
        TRY(UADC_SetSampleRate(unit, priv->cfg.frequency));

        LL_TIM_EnableARRPreload(priv->TIMx);
        LL_TIM_EnableUpdateEvent(priv->TIMx);
        LL_TIM_SetTriggerOutput(priv->TIMx, LL_TIM_TRGO_UPDATE);
        LL_TIM_GenerateEvent_UPDATE(priv->TIMx); // load the prescaller value
    }

    // --------------------- CONFIGURE THE ADC ---------------------------
    adc_dbg("Setting up ADC");
    {
        // Calibrate the ADC
        adc_dbg("Wait for calib");
        LL_ADC_StartCalibration(priv->ADCx);
        while (LL_ADC_IsCalibrationOnGoing(priv->ADCx)) {}
        adc_dbg("ADC calibrated.");

        // Let's just enable the internal channels always - makes toggling them on-line easier
        LL_ADC_SetCommonPathInternalCh(priv->ADCx_Common, LL_ADC_PATH_INTERNAL_VREFINT | LL_ADC_PATH_INTERNAL_TEMPSENSOR);

        LL_ADC_SetDataAlignment(priv->ADCx, LL_ADC_DATA_ALIGN_RIGHT);
        LL_ADC_SetResolution(priv->ADCx, LL_ADC_RESOLUTION_12B);
        LL_ADC_REG_SetDMATransfer(priv->ADCx, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);

        // configure channels
        priv->channels_mask = priv->cfg.channels;

        priv->ADCx->CHSELR = priv->channels_mask;

        LL_ADC_REG_SetTriggerSource(priv->ADCx, LL_ADC_REG_TRIG_EXT_TIM15_TRGO);

        LL_ADC_SetSamplingTimeCommonChannels(priv->ADCx, LL_ADC_SAMPLETIMES[priv->cfg.sample_time]);

        // will be enabled when switching to INIT mode
    }

    // --------------------- CONFIGURE DMA -------------------------------
    UADC_SetupDMA(unit);

    // prepare the avg factor float for the ISR
    if (priv->cfg.averaging_factor > 1000) priv->cfg.averaging_factor = 1000; // normalize
    priv->avg_factor_as_float = priv->cfg.averaging_factor/1000.0f;

    adc_dbg("ADC peripherals configured.");

    irqd_attach(priv->DMA_CHx, UADC_DMA_Handler, unit);
    irqd_attach(priv->ADCx, UADC_ADC_EOS_Handler, unit);
    adc_dbg("irqs attached");

    UADC_SwitchMode(unit, ADC_OPMODE_IDLE);
    adc_dbg("ADC done");

    return E_SUCCESS;
}

/** Tear down the unit */
void UADC_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init peripherals
    if (unit->status == E_SUCCESS ) {
        UADC_SwitchMode(unit, ADC_OPMODE_UNINIT);

        //LL_ADC_DeInit(priv->ADCx);
        LL_ADC_CommonDeInit(priv->ADCx_Common);
        LL_TIM_DeInit(priv->TIMx);

        irqd_detach(priv->DMA_CHx, UADC_DMA_Handler);
        irqd_detach(priv->ADCx, UADC_ADC_EOS_Handler);

        LL_DMA_DeInit(priv->DMAx, priv->dma_chnum);
    }

    // free buffer if not NULL
    free_ck(priv->dma_buffer);

    // Release all resources, deinit pins
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
