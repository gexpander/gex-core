//
// Created by MightyPork on 2018/02/03.
//

#include <stm32f072xb.h>
#include "platform.h"
#include "unit_base.h"

#define ADC_INTERNAL
#include "_adc_internal.h"

/** Allocate data structure and set defaults */
error_t UADC_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    priv->channels = 1; // PA0
    priv->enable_tsense = false;
    priv->enable_vref = false;
    priv->sample_time = 0b010; // 13.5c
    priv->frequency = 1000;
    priv->buffer_size = 512;
    priv->averaging_factor = 500;

    priv->opmode = ADC_OPMODE_UNINIT;

    return E_SUCCESS;
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
        for (uint8_t i = 0; i < 16; i++) {
            if (priv->channels & (1 << i)) {
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
                else {
                    c = 'C';
                    num = (uint8_t) (i - 10);
                }

                TRY(rsc_claim_pin(unit, c, num));
                uint32_t ll_pin = hw_pin2ll(num, &suc);
                GPIO_TypeDef *port = hw_port2periph(c, &suc);
                assert_param(suc);

                LL_GPIO_SetPinPull(port, ll_pin, LL_GPIO_PULL_NO);
                LL_GPIO_SetPinMode(port, ll_pin, LL_GPIO_MODE_ANALOG);
                priv->nb_channels++;
            }
        }
        if (priv->enable_tsense) priv->nb_channels++;
        if (priv->enable_vref) priv->nb_channels++;

        if (priv->nb_channels == 0) {
            dbg("!! Need at least 1 channel");
            return E_BAD_CONFIG;
        }

        if (priv->buffer_size < priv->nb_channels*2*2) {
            dbg("Insufficient buf size");
            return E_BAD_CONFIG;
        }
    }

    // ------------------- ENABLE CLOCKS --------------------------
    {
        // enable peripherals clock
        hw_periph_clock_enable(priv->ADCx);
        hw_periph_clock_enable(priv->TIMx);
        // DMA and GPIO clocks are enabled on startup automatically
    }

    // ------------------- CONFIGURE THE TIMER --------------------------
    dbg("Setting up TIMER");
    {
        // Find suitable timer values
        uint16_t presc;
        uint32_t count;
        float real_freq;
        if (!solve_timer(PLAT_APB1_HZ, priv->frequency, true, &presc, &count, &real_freq)) {
            dbg("Failed to resolve timer params.");
            return E_BAD_VALUE;
        }
        dbg("Frequency error %d ppm, presc %d, count %d",
            (int) lrintf(1000000.0f * ((real_freq - priv->frequency) / (float)priv->frequency)), (int) presc, (int) count);

        LL_TIM_SetPrescaler(priv->TIMx, (uint32_t) (presc - 1));
        LL_TIM_SetAutoReload(priv->TIMx, count - 1);
        LL_TIM_EnableARRPreload(priv->TIMx);
        LL_TIM_EnableUpdateEvent(priv->TIMx);
        LL_TIM_SetTriggerOutput(priv->TIMx, LL_TIM_TRGO_UPDATE);
        LL_TIM_GenerateEvent_UPDATE(priv->TIMx); // load the prescaller value
    }

    // --------------------- CONFIGURE THE ADC ---------------------------
    dbg("Setting up ADC");
    {
        // Calibrate the ADC
        dbg("Wait for calib");
        LL_ADC_StartCalibration(priv->ADCx);
        while (LL_ADC_IsCalibrationOnGoing(priv->ADCx)) {}
        dbg("ADC calibrated.");

        {
            uint32_t mask = 0;
            if (priv->enable_vref) mask |= LL_ADC_PATH_INTERNAL_VREFINT;
            if (priv->enable_tsense) mask |= LL_ADC_PATH_INTERNAL_TEMPSENSOR;
            LL_ADC_SetCommonPathInternalCh(priv->ADCx_Common, mask);
        }

        LL_ADC_SetDataAlignment(priv->ADCx, LL_ADC_DATA_ALIGN_RIGHT);
        LL_ADC_SetResolution(priv->ADCx, LL_ADC_RESOLUTION_12B);
        LL_ADC_REG_SetDMATransfer(priv->ADCx, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);

        // configure channels
        priv->extended_channels_mask = priv->channels;
        if (priv->enable_tsense) priv->extended_channels_mask |= (1<<16);
        if (priv->enable_vref) priv->extended_channels_mask |= (1<<17);

        priv->ADCx->CHSELR = priv->extended_channels_mask;

        LL_ADC_REG_SetTriggerSource(priv->ADCx, LL_ADC_REG_TRIG_EXT_TIM15_TRGO);

        LL_ADC_SetSamplingTimeCommonChannels(priv->ADCx, LL_ADC_SAMPLETIMES[priv->sample_time]);

//        LL_ADC_Enable(priv->ADCx);
    }

    // --------------------- CONFIGURE DMA -------------------------------
    dbg("Setting up DMA");
    {
        // The length must be a 2*multiple of the number of channels, in bytes
        uint16_t itemcount = (uint16_t) ((priv->nb_channels) * (uint16_t) (priv->buffer_size / (2 * priv->nb_channels)));
        if (itemcount % 2 == 1) itemcount -= priv->nb_channels;
        priv->dma_buffer_itemcount = itemcount;

        dbg("DMA item count is %d (%d bytes), There are %d 2-byte samples per group.",
            priv->dma_buffer_itemcount,
            priv->dma_buffer_itemcount*sizeof(uint16_t),
            priv->nb_channels);

        priv->dma_buffer = malloc_ck(priv->dma_buffer_itemcount * sizeof(uint16_t));
        if (NULL == priv->dma_buffer) return E_OUT_OF_MEM;
        assert_param(((uint32_t) priv->dma_buffer & 3) == 0); // must be aligned

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

            // Interrupt on transfer 1/2 and complete
            // We will capture the first and second half and send it while the other half is being filled.
//            LL_DMA_EnableIT_HT(priv->DMAx, priv->dma_chnum);
//            LL_DMA_EnableIT_TC(priv->DMAx, priv->dma_chnum);
        }

        LL_DMA_EnableChannel(priv->DMAx, priv->dma_chnum);
    }

    // prepare the avg factor float for the ISR
    if (priv->averaging_factor > 1000) priv->averaging_factor = 1000; // normalize
    priv->avg_factor_as_float = priv->averaging_factor/1000.0f;

    dbg("ADC peripherals configured.");

    irqd_attach(priv->DMA_CHx, UADC_DMA_Handler, unit);
    irqd_attach(priv->ADCx, UADC_ADC_EOS_Handler, unit);

    UADC_SwitchMode(unit, ADC_OPMODE_IDLE);

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

        free_ck(priv->dma_buffer);
    }

    // Release all resources, deinit pins
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}