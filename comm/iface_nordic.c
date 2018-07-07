//
// Created by MightyPork on 2018/04/06.
//

#include "platform.h"
#if SUPPORT_NRF

#include "iface_nordic.h"
#include "nrf_pins.h"
#include "resources.h"
#include "hw_utils.h"
#include "nrf.h"
#include "unit_base.h"
#include "system_settings.h"
#include "utils/hexdump.h"


extern osSemaphoreId semVcomTxReadyHandle;

#define RX_PIPE_NUM 0

void iface_nordic_claim_resources(void)
{
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, NRF_R_SPI));
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, NRF_R_CE));
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, NRF_R_NSS));
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, NRF_R_IRQ));
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, NRF_R_MISO));
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, NRF_R_MOSI));
    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, NRF_R_SCK));

    assert_param(E_SUCCESS == rsc_claim(&UNIT_SYSTEM, R_EXTI0+NRF_EXTI_LINENUM));
}

void iface_nordic_free_resources(void)
{
    rsc_free(&UNIT_SYSTEM, NRF_R_SPI);
    rsc_free(&UNIT_SYSTEM, NRF_R_CE);
    rsc_free(&UNIT_SYSTEM, NRF_R_NSS);
    rsc_free(&UNIT_SYSTEM, NRF_R_IRQ);
    rsc_free(&UNIT_SYSTEM, NRF_R_MISO);
    rsc_free(&UNIT_SYSTEM, NRF_R_MOSI);
    rsc_free(&UNIT_SYSTEM, NRF_R_SCK);

    rsc_free(&UNIT_SYSTEM, R_EXTI0+NRF_EXTI_LINENUM);
}

static uint8_t rx_buffer[32];

static void NrfIrqHandler(void *arg)
{
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINES[NRF_EXTI_LINENUM]);
    dbg_nrf("[EXTI] ---");

    while (NRF_IsRxPacket()) {
        uint8_t pipenum;
        uint8_t count = NRF_ReceivePacket(rx_buffer, &pipenum);
        if (count > 0) {
            dbg_nrf("NRF RX %d bytes", (int) count);
            rxQuePostMsg(rx_buffer, count);
        }
        else {
            dbg("IRQ but no Rx");
        }
    }

    dbg_nrf("--- end [EXTI]");
}

bool iface_nordic_init(void)
{
    dbg("Setting up Nordic...");

    hw_periph_clock_enable(NRF_SPI);

    // SPI pins
    assert_param(E_SUCCESS == hw_configure_gpiorsc_af(NRF_R_SCK, NRF_SCK_AF));
    assert_param(E_SUCCESS == hw_configure_gpiorsc_af(NRF_R_MOSI, NRF_MOSI_AF));
    assert_param(E_SUCCESS == hw_configure_gpiorsc_af(NRF_R_MISO, NRF_MISO_AF));

    // Manual pins
    LL_GPIO_SetPinMode(NRF_NSS_GPIO_Port, NRF_NSS_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(NRF_CE_GPIO_Port, NRF_CE_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(NRF_RST_GPIO_Port, NRF_RST_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(NRF_IRQ_GPIO_Port, NRF_IRQ_Pin, LL_GPIO_MODE_INPUT);

    // set up SPI
    LL_SPI_Disable(NRF_SPI);
    {
        LL_SPI_SetBaudRatePrescaler(NRF_SPI, LL_SPI_BAUDRATEPRESCALER_DIV32);
        //LL_SPI_BAUDRATEPRESCALER_DIV8

        LL_SPI_SetClockPolarity(NRF_SPI,     LL_SPI_POLARITY_LOW);
        LL_SPI_SetClockPhase(NRF_SPI,        LL_SPI_PHASE_1EDGE);
        LL_SPI_SetTransferDirection(NRF_SPI, LL_SPI_FULL_DUPLEX);
        LL_SPI_SetTransferBitOrder(NRF_SPI,  LL_SPI_MSB_FIRST);

        LL_SPI_SetNSSMode(NRF_SPI, LL_SPI_NSS_SOFT);
        LL_SPI_SetDataWidth(NRF_SPI, LL_SPI_DATAWIDTH_8BIT);
        LL_SPI_SetRxFIFOThreshold(NRF_SPI, LL_SPI_RX_FIFO_TH_QUARTER); // trigger RXNE on 1 byte

        LL_SPI_SetMode(NRF_SPI, LL_SPI_MODE_MASTER);
    }
    LL_SPI_Enable(NRF_SPI);

    // reset the radio / enable its power supply
    NRF_Reset(1);
    LL_mDelay(5);
    NRF_Reset(0);

    dbg("configure nrf module");

    // Now configure the radio
    if (!NRF_Init(NRF_SPEED_2M)) {// TODO configurable speed - also maybe better to use slower
        dbg("--- NRF not present!");
        return false;
    }

    NRF_SetChannel(SystemSettings.nrf_channel);
    NRF_SetBaseAddress(SystemSettings.nrf_network);
    NRF_SetRxAddress(RX_PIPE_NUM, SystemSettings.nrf_address);
    NRF_EnablePipe(RX_PIPE_NUM);
    NRF_ModeRX();

    dbg("enable exti");
    // EXTI
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINES[NRF_EXTI_LINENUM]);
    LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINES[NRF_EXTI_LINENUM]);
    LL_SYSCFG_SetEXTISource(NRF_SYSCFG_EXTI_PORT, LL_SYSCFG_EXTI_LINES[NRF_EXTI_LINENUM]);
    irqd_attach(EXTIS[NRF_EXTI_LINENUM], NrfIrqHandler, NULL);
    // TODO increase priority in NVIC?

    dbg("nrf setup done");
    return true;
}

void iface_nordic_deinit(void)
{
    LL_EXTI_DisableIT_0_31(LL_EXTI_LINES[NRF_EXTI_LINENUM]);
    LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINES[NRF_EXTI_LINENUM]);
    irqd_detach(EXTIS[NRF_EXTI_LINENUM], NrfIrqHandler);
    hw_periph_clock_disable(NRF_SPI);

    hw_deinit_pin_rsc(NRF_R_SCK);
    hw_deinit_pin_rsc(NRF_R_MOSI);
    hw_deinit_pin_rsc(NRF_R_MISO);
    hw_deinit_pin_rsc(NRF_R_NSS);
    hw_deinit_pin_rsc(NRF_R_CE);
    hw_deinit_pin_rsc(NRF_R_IRQ);
    hw_deinit_pin_rsc(NRF_R_RST);
}

// FIXME
#define MAX_RETRY 5

void iface_nordic_transmit(const uint8_t *buff, uint32_t len)
{
    bool suc = false;
    while (len > 0) {
        uint8_t chunk = (uint8_t) MIN(32, len);

        uint16_t delay = 1;
        //hexDump("Tx chunk", buff, chunk);
        for (int i = 0; i < MAX_RETRY; i++) {
            suc = NRF_SendPacket(RX_PIPE_NUM, buff, chunk); // use the pipe to retrieve the address
            if (!suc) {
                vTaskDelay(delay);
                delay *= 2; // longer delay next time
            } else {
                break;
            }
        }

        if (!suc) {
            break;
        }

        buff += chunk;
        len -= chunk;
    }

    if (suc) {
        dbg_nrf("+ NRF Tx OK!");
    } else {
        dbg("- NRF sending failed");
    }

    // give when it's done
    xSemaphoreGive(semVcomTxReadyHandle); // similar to how it's done in USB - this is called in the Tx Done handler
}

#endif // SUPPORT_NRF
