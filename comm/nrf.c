//
// Created by MightyPork on 2018/04/02.
//

#include "platform.h"
#if SUPPORT_NRF

#include "nrf.h"

/**
 * Read a register
 *
 * @param reg - register number (max 83)
 * @return register value
 */
static uint8_t NRF_ReadRegister(uint8_t reg);

/**
 * Write a register
 *
 * @param reg - register number (max 83)
 * @param value - register value
 * @return status register value
 */
static uint8_t NRF_WriteRegister(uint8_t reg, uint8_t value);

// Internal use
static uint8_t NRF_WriteBuffer(uint8_t reg, const uint8_t *pBuf, uint8_t bytes);

static uint8_t NRF_ReadBuffer(uint8_t reg, uint8_t *pBuf, uint8_t bytes);

/**
 * Set TX address for next frame
 *
 * @param SendTo - addr
 */
static void NRF_SetTxAddress(uint8_t ToAddr);

//----------------------------------------------------------

/** Tight asm loop */
#define __asm_loop(cycles) \
do { \
    register uint32_t _count asm ("r4") = cycles; \
    asm volatile( \
        ".syntax unified\n" \
        ".thumb\n" \
        "0:" \
            "subs %0, #1\n" \
            "bne 0b\n" \
        : "+r" (_count)); \
} while(0)

// 12 for 72 MHz
#define _delay_us(n) __asm_loop((n)*8)

static inline void CE(bool level) {
    __asm_loop(1);
    if (level) LL_GPIO_SetOutputPin(NRF_CE_GPIO_Port, NRF_CE_Pin);
    else LL_GPIO_ResetOutputPin(NRF_CE_GPIO_Port, NRF_CE_Pin);
    __asm_loop(1);
}

static inline void NSS(bool level) {
    __asm_loop(1);
    if (level) LL_GPIO_SetOutputPin(NRF_NSS_GPIO_Port, NRF_NSS_Pin);
    else LL_GPIO_ResetOutputPin(NRF_NSS_GPIO_Port, NRF_NSS_Pin);
    __asm_loop(1);
}

static uint8_t spi(uint8_t tx) {
    while (! LL_SPI_IsActiveFlag_TXE(NRF_SPI));
    LL_SPI_TransmitData8(NRF_SPI, tx);
    while (! LL_SPI_IsActiveFlag_RXNE(NRF_SPI));
    return LL_SPI_ReceiveData8(NRF_SPI);
}

void NRF_Reset(bool enable_reset)
{
    if (enable_reset) {
        // go HIGH - this closes the PMOS
        LL_GPIO_SetOutputPin(NRF_RST_GPIO_Port, NRF_RST_Pin);
    } else {
        // LOW - open PMOS, enable power
        LL_GPIO_ResetOutputPin(NRF_RST_GPIO_Port, NRF_RST_Pin);
    }
}

//-------

/*
 * from: http://barefootelectronics.com/NRF24L01.aspx
 */

#define CMD_READ_REG        0x00  // Read command to register
#define CMD_WRITE_REG       0x20  // Write command to register
#define CMD_RD_RX_PLD     0x61  // Read RX payload register address
#define CMD_WR_TX_PLD     0xA0  // Write TX payload register address
#define CMD_FLUSH_TX        0xE1  // Flush TX register command
#define CMD_FLUSH_RX        0xE2  // Flush RX register command
#define CMD_REUSE_TX_PL     0xE3  // Reuse TX payload register command
#define CMD_RD_RX_PL_WIDTH    0x60  // Read RX Payload Width
#define CMD_WR_ACK_PLD   0xA8  // Write ACK payload for Pipe (Add pipe number 0-6)
#define CMD_WR_TX_PLD_NK  0xB0  // Write ACK payload for not ack
#define CMD_NOP             0xFF  // No Operation, might be used to read status register

// SPI(nRF24L01) registers(addresses)
#define RG_CONFIG          0x00  // 'Config' register address
#define RG_EN_AA           0x01  // 'Enable Auto Acknowledgment' register address
#define RG_EN_RXADDR       0x02  // 'Enabled RX addresses' register address
#define RG_SETUP_AW        0x03  // 'Setup address width' register address
#define RG_SETUP_RETR      0x04  // 'Setup Auto. Retrans' register address
#define RG_RF_CH           0x05  // 'RF channel' register address
#define RG_RF_SETUP        0x06  // 'RF setup' register address
#define RG_STATUS          0x07  // 'Status' register address
#define RG_OBSERVE_TX      0x08  // 'Observe TX' register address
#define RG_CD              0x09  // 'Carrier Detect' register address
#define RG_RX_ADDR_P0      0x0A  // 'RX address pipe0' register address
#define RG_RX_ADDR_P1      0x0B  // 'RX address pipe1' register address
#define RG_RX_ADDR_P2      0x0C  // 'RX address pipe2' register address
#define RG_RX_ADDR_P3      0x0D  // 'RX address pipe3' register address
#define RG_RX_ADDR_P4      0x0E  // 'RX address pipe4' register address
#define RG_RX_ADDR_P5      0x0F  // 'RX address pipe5' register address
#define RG_TX_ADDR         0x10  // 'TX address' register address
#define RG_RX_PW_P0        0x11  // 'RX payload width, pipe0' register address
#define RG_RX_PW_P1        0x12  // 'RX payload width, pipe1' register address
#define RG_RX_PW_P2        0x13  // 'RX payload width, pipe2' register address
#define RG_RX_PW_P3        0x14  // 'RX payload width, pipe3' register address
#define RG_RX_PW_P4        0x15  // 'RX payload width, pipe4' register address
#define RG_RX_PW_P5        0x16  // 'RX payload width, pipe5' register address
#define RG_FIFO_STATUS     0x17  // 'FIFO Status Register' register address
#define RG_DYNPD           0x1C  // 'Enable dynamic payload length
#define RG_FEATURE         0x1D  // 'Feature register

#define RD_STATUS_TX_FULL 0x01 // tx queue full
#define RD_STATUS_RX_PNO  0x0E // pipe number of the received payload
#define RD_STATUS_MAX_RT  0x10 // max retransmissions
#define RD_STATUS_TX_DS   0x20 // data sent
#define RD_STATUS_RX_DR   0x40 // data ready

#define RD_CONFIG_PRIM_RX 0x01 // is primary receiver
#define RD_CONFIG_PWR_UP  0x02 // power up
#define RD_CONFIG_CRCO    0x04 // 2-byte CRC
#define RD_CONFIG_EN_CRC  0x08 // enable CRC
#define RD_CONFIG_DISABLE_IRQ_MAX_RT 0x10
#define RD_CONFIG_DISABLE_IRQ_TX_DS 0x20
#define RD_CONFIG_DISABLE_IRQ_RX_DR 0x40

#define RD_FIFO_STATUS_RX_EMPTY 0x01
#define RD_FIFO_STATUS_RX_FULL 0x02
#define RD_FIFO_STATUS_TX_EMPTY 0x10
#define RD_FIFO_STATUS_TX_FULL 0x20
#define RD_FIFO_STATUS_TX_REUSE 0x40

// Config register bits (excluding the bottom two that are changed dynamically)
// enable only Rx IRQ
#define ModeBits (RD_CONFIG_DISABLE_IRQ_MAX_RT | \
                  RD_CONFIG_DISABLE_IRQ_TX_DS | \
                  RD_CONFIG_EN_CRC | \
                  RD_CONFIG_CRCO)

static inline uint8_t CS(uint8_t hl)
{
    if (hl == 1) {
        NSS(0);
    }
    else {
        NSS(1);
    }
    return hl;
}

#define CHIPSELECT for (uint8_t cs = CS(1); cs==1; cs = CS(0))

// Base NRF address - byte 4 may be modified before writing to a pipe register,
// the first 4 bytes are shared in the network. Master is always code 0.
static uint8_t nrf_base_address[5];

static uint8_t nrf_pipe_addr[6];
static bool nrf_pipe_enabled[6];

static uint8_t NRF_WriteRegister(uint8_t reg, uint8_t value)
{
    uint8_t status = 0;
    CHIPSELECT {
        status = spi(CMD_WRITE_REG | reg);
        spi(value);
    }
    dbg_nrf("Wr[0x%02x] := 0x%02x", (int)reg, (int) value);


    uint8_t reg_val = 0;
    CHIPSELECT {
        spi(CMD_READ_REG | reg);
        reg_val = spi(0);
    }
    dbg_nrf("     verify 0x%02x", (int)reg_val);
    if (reg_val != value)
        dbg_nrf("                           !!!");
    else
        dbg_nrf("                           OK");


    return status;
}

static uint8_t NRF_ReadRegister(uint8_t reg)
{
    uint8_t reg_val = 0;
    CHIPSELECT {
        spi(CMD_READ_REG | reg);
        reg_val = spi(0);
    }
    dbg_nrf("Rd[0x%02x]  = 0x%02x", (int)reg, (int) reg_val);
    return reg_val;
}

static uint8_t NRF_ReadStatus(void)
{
    uint8_t reg_val = 0;
    CHIPSELECT {
        reg_val = spi(CMD_NOP);
    }
    return reg_val;
}

static uint8_t NRF_WriteBuffer(uint8_t reg, const uint8_t *pBuf, uint8_t bytes)
{
    uint8_t status = 0, i = 0;
    CHIPSELECT {
        status = spi(CMD_WRITE_REG | reg);
        for (i = 0; i < bytes; i++) {
            spi(*pBuf++);
        }
    }
    return status;
}

static uint8_t NRF_ReadBuffer(uint8_t reg, uint8_t *pBuf, uint8_t bytes)
{
    uint8_t status = 0, i;
    CHIPSELECT {
        status = spi(CMD_READ_REG | reg);
        for (i = 0; i < bytes; i++) {
            pBuf[i] = spi(0);
        }
    }
    return status;
}

void NRF_SetBaseAddress(const uint8_t *Bytes4)
{
    memcpy(nrf_base_address, Bytes4, 4);
    nrf_base_address[4] = 0;

    // write it to the two full-width address registers
    NRF_WriteBuffer(RG_RX_ADDR_P0, nrf_base_address, 5);
    nrf_base_address[4] = 1;// to be different
    NRF_WriteBuffer(RG_RX_ADDR_P1, nrf_base_address, 5);
}

void NRF_SetRxAddress(uint8_t pipenum, uint8_t AddrByte)
{
    if (pipenum > 5) {
        dbg_nrf("!! bad pipe %d", pipenum);
        return;
    }

    nrf_base_address[4] = AddrByte;

    nrf_pipe_addr[pipenum] = AddrByte;

    dbg_nrf("Set Rx addr (pipe %d) = 0x%02x", (int)pipenum, AddrByte);
    if (pipenum == 0) {
        dbg_nrf("W ADDR_PA0: %02X-%02X-%02X-%02X-%02X", nrf_base_address[0], nrf_base_address[1], nrf_base_address[2], nrf_base_address[3], nrf_base_address[4]);
        NRF_WriteBuffer(RG_RX_ADDR_P0, nrf_base_address, 5);
    }
    else if (pipenum == 1) {
        dbg_nrf("W ADDR_PA1: %02X-%02X-%02X-%02X-%02X", nrf_base_address[0], nrf_base_address[1], nrf_base_address[2], nrf_base_address[3], nrf_base_address[4]);
        NRF_WriteBuffer(RG_RX_ADDR_P1, nrf_base_address, 5);
    }
    else {
        NRF_WriteRegister(RG_RX_ADDR_P2 + (pipenum - 2), AddrByte);
    }
}

bool NRF_AddPipe(uint8_t AddrByte, uint8_t *PipeNum)
{
    for (uint8_t i = 0; i < 6; i++) {
        if(!nrf_pipe_enabled[i]) {
            NRF_SetRxAddress(i, AddrByte);
            *PipeNum = i;
            NRF_EnablePipe(i);
            return true;
        }
    }

    return false;
}

uint8_t NRF_PipeNum2Addr(uint8_t pipe_num)
{
    if (pipe_num > 5) return 0; // fail
    return nrf_pipe_addr[pipe_num];
}

uint8_t NRF_Addr2PipeNum(uint8_t addr)
{
    for (int i = 0; i < 6; i++) {
        if (nrf_pipe_addr[i] == addr) return (uint8_t) i;
    }
    return 0xFF;
}

void NRF_EnablePipe(uint8_t pipenum)
{
    dbg_nrf("Enable pipe num %d", (int)pipenum);
    uint8_t enabled = NRF_ReadRegister(RG_EN_RXADDR);
    enabled |= 1 << pipenum;
    NRF_WriteRegister(RG_EN_RXADDR, enabled);

    nrf_pipe_enabled[pipenum] = 1;
}

void NRF_DisablePipe(uint8_t pipenum)
{
    uint8_t enabled = NRF_ReadRegister(RG_EN_RXADDR);
    enabled &= ~(1 << pipenum);
    NRF_WriteRegister(RG_EN_RXADDR, enabled);

    nrf_pipe_enabled[pipenum] = 0;
}

static void NRF_SetTxAddress(uint8_t SendTo)
{
    nrf_base_address[4] = SendTo;

    dbg_nrf("W Tx_ADDR + Rx0: %02X-%02X-%02X-%02X-%02X",
        nrf_base_address[0], nrf_base_address[1], nrf_base_address[2], nrf_base_address[3], nrf_base_address[4]);

    NRF_WriteBuffer(RG_TX_ADDR, nrf_base_address, 5);
    NRF_WriteBuffer(RG_RX_ADDR_P0, nrf_base_address, 5); // the ACK will come to pipe 0
}

void NRF_PowerDown(void)
{
    dbg_nrf("PDn");
    CE(0);
    NRF_WriteRegister(RG_CONFIG, ModeBits);
}

void NRF_ModeTX(void)
{
    dbg_nrf("Tx Mode");

    CE(0);
    uint8_t m = NRF_ReadRegister(RG_CONFIG);
    NRF_WriteRegister(RG_CONFIG, ModeBits | RD_CONFIG_PWR_UP);
    if ((m & RD_CONFIG_PWR_UP) == 0) {
        // switching on
        LL_mDelay(5);
    }
}

void NRF_ModeRX(void)
{
    dbg_nrf("Rx Mode");
    NRF_WriteRegister(RG_CONFIG, ModeBits | RD_CONFIG_PWR_UP | RD_CONFIG_PRIM_RX);
    NRF_SetRxAddress(0, nrf_pipe_addr[0]); // set the P0 address - it was changed during Rx for ACK reception
    CE(1);

    //if ((m&2)==0) LL_mDelay()(5); You don't need to wait. Just nothing will come for 5ms or more
}

uint8_t NRF_IsModePowerDown(void)
{
    uint8_t ret = 0;
    if ((NRF_ReadRegister(RG_CONFIG) & RD_CONFIG_PWR_UP) == 0) ret = 1;
    return ret;
}

uint8_t NRF_IsModeTX(void)
{
    uint8_t m = NRF_ReadRegister(RG_CONFIG);
    if ((m & RD_CONFIG_PWR_UP) == 0) return 0; // OFF
    if ((m & RD_CONFIG_PRIM_RX) == 0) return 1;
    return 0;
}

uint8_t NRF_IsModeRx(void)
{
    uint8_t m = NRF_ReadRegister(RG_CONFIG);
    if ((m & RD_CONFIG_PWR_UP) == 0) return 0; // OFF
    if ((m & RD_CONFIG_PRIM_RX) == 0) return 0;
    return 1;
}

void NRF_SetChannel(uint8_t Ch)
{
    NRF_WriteRegister(RG_RF_CH, Ch);
}

uint8_t NRF_ReceivePacket(uint8_t *Packet, uint8_t *PipeNum)
{
    uint8_t pw = 0, status;
//    if (!NRF_IsRxPacket()) {
//        dbg("rx queue empty");
//        return 0;
//    }

//    const uint8_t orig_conf = NRF_ReadRegister(RG_CONFIG);
//    CE(0); // quit Rx mode - go idle

    CHIPSELECT {
        status = spi(CMD_RD_RX_PL_WIDTH);
        pw = spi(0);
    }

    if (pw == 0) {
        dbg("empty pld");
    }

    if (pw > 32) {
        CHIPSELECT {
            spi(CMD_FLUSH_RX);
        }
        pw = 0;
        dbg("over 32");
    } else {
        // Read the reception pipe number
        *PipeNum = ((status & RD_STATUS_RX_PNO) >> 1);
        CHIPSELECT {
            spi(CMD_RD_RX_PLD);
            for (uint8_t i = 0; i < pw; i++) Packet[i] = spi(0);
        }
    }
    NRF_WriteRegister(RG_STATUS, RD_STATUS_RX_DR); // Clear the RX_DR interrupt

//    if ((orig_conf & RD_CONFIG_PWR_UP) == 0) {
//        dbg_nrf("going back PwrDn");
//        NRF_PowerDown();
//    }
//    else if ((orig_conf & RD_CONFIG_PRIM_RX) == RD_CONFIG_PRIM_RX) {
//        dbg_nrf("going back PwrUp+Rx");
//        NRF_ModeRX();
//    }
//    CE(1); // back to rx
    return pw;
}

bool NRF_IsRxPacket(void)
{
    return 0 == (NRF_ReadRegister(RG_FIFO_STATUS) & RD_FIFO_STATUS_RX_EMPTY);
//    uint8_t ret = NRF_ReadRegister(RG_STATUS) & RD_STATUS_RX_DR;
//    return 0 != ret;
}

bool NRF_SendPacket(uint8_t PipeNum, const uint8_t *Packet, uint8_t Length)
{
    if (!nrf_pipe_enabled[PipeNum]) {
        dbg_nrf("!! Pipe %d not enabled", PipeNum);
        return 0;
    }

    dbg_nrf("Will tx to addr %02x", nrf_pipe_addr[PipeNum]);

    const uint8_t orig_conf = NRF_ReadRegister(RG_CONFIG);
    CE(0);
    NRF_ModeTX();        // Make sure in TX mode
    NRF_SetTxAddress(nrf_pipe_addr[PipeNum]); // this sets the Tx addr and also pipe 0 addr for ACK

    CHIPSELECT {
        spi(CMD_FLUSH_TX);
    };

    CHIPSELECT {
        spi(CMD_WR_TX_PLD);
        for (uint8_t i = 0; i < Length; i++) spi(Packet[i]);
    };

    // CE pulse
    CE(1);
    _delay_us(20); // At least 10 us
    CE(0);

    uint8_t st = 0;
    while ((st & (RD_STATUS_MAX_RT|RD_STATUS_TX_DS)) == 0) {
        st = NRF_ReadStatus(); // Packet acked or timed out
    }

    dbg_nrf("Send status: 0x%02x - MAX_RT %d, SENT %d", (int)st,
        (st&RD_STATUS_MAX_RT) != 0, (st&RD_STATUS_TX_DS) != 0);

    NRF_WriteRegister(RG_STATUS, st & (RD_STATUS_MAX_RT|RD_STATUS_TX_DS)); // Clear the bit

    if ((orig_conf & RD_CONFIG_PWR_UP) == 0) {
        dbg_nrf("going back PwrDn");
        NRF_PowerDown();
    }
    else if ((orig_conf & RD_CONFIG_PRIM_RX) == RD_CONFIG_PRIM_RX) {
        dbg_nrf("going back PwrUp+Rx");
        NRF_ModeRX();
    }

    return 0 != (st & RD_STATUS_TX_DS); // success
}

bool NRF_Init(uint8_t pSpeed)
{
    // Set the required output pins
    NSS(1);
    CE(0);

    LL_mDelay(200);

    for (int i = 0; i < 6; i++) {
        nrf_pipe_addr[i] = 0;
        nrf_pipe_enabled[i] = 0;
    }

    // this is a test if there's anything present
    uint8_t awreg = NRF_ReadRegister(RG_SETUP_AW);
    if (awreg == 0 || awreg > 3) return false;

    // clear flags etc
    NRF_PowerDown();
    CHIPSELECT { spi(CMD_FLUSH_RX); }
    CHIPSELECT { spi(CMD_FLUSH_TX); }
    NRF_WriteRegister(RG_STATUS, 0x70);

    NRF_WriteRegister(RG_CONFIG, ModeBits);
    NRF_WriteRegister(RG_SETUP_AW, 0b11);             // 5 byte addresses

    NRF_WriteRegister(RG_EN_RXADDR, 0x01); // disable all, enable pipe 0 - this is required for shockburst, despite not being specified in the DS

    NRF_WriteRegister(RG_SETUP_RETR, 0x18);        // 8 retries
    NRF_WriteRegister(RG_RF_CH, 2);                // channel 2 NO HIGHER THAN 83 in USA!

    NRF_WriteRegister(RG_RF_SETUP, pSpeed);

    NRF_WriteRegister(RG_DYNPD, 0b111111);         // Dynamic packet length
    NRF_WriteRegister(RG_FEATURE, 0b100);          // Enable dynamic payload, and no payload in the ack.

//    for (int i = 0; i < 6; i++) {
//        NRF_WriteRegister(RG_RX_PW_P0+i, 32);      // Receive 32 byte packets - XXX this is probably not needed with dynamic length
//    }

    return true;
}

#endif
