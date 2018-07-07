//
// Created by MightyPork on 2018/04/02.
//

#ifndef GEX_NRF_NRF_H
#define GEX_NRF_NRF_H

/*
 * nordic.h
 *
 * Created:12/16/2013 3:36:04 PM
 *  Author: Tom
 *
 *    NRF24L01+ Library II
 *
 */

#include "platform.h"
#if SUPPORT_NRF

#include "resources.h"
#include "nrf_pins.h"

#define dbg_nrf(...) do{}while(0)
//#define dbg_nrf dbg

// Initialize SPI and the Nordic

/**
 * Initialize the NRF module
 *
 * @param pSpeed
 * @return success (0 if device not detected)
 */
bool NRF_Init(uint8_t pSpeed);

#define NRF_SPEED_500k 0b00100110
#define NRF_SPEED_2M 0b00001110
#define NRF_SPEED_1M 0b00000110

/**
 * Set the reset pin (this is a PMOS controlling its power)
 *
 * @param enable_reset 1 to go into reset, 0 to enter operational state (will need some time to become ready)
 */
void NRF_Reset(bool enable_reset);

/**
 * Set reception address
 * @param pipenum - pipe to set
 * @param AddrByte - byte 0
 */
void NRF_SetRxAddress(uint8_t pipenum, uint8_t AddrByte);

/**
 * Set communication channel
 */
void NRF_SetChannel(uint8_t Ch) ; // 0 through 83 only!

/**
 * Power down the transceiver
 */
void NRF_PowerDown(void);

/**
 * Selecr RX mode
 */
void NRF_ModeRX(void);

/**
 * Select TX mode
 */
void NRF_ModeTX(void);

/**
 * @return NRF is power down
 */
uint8_t NRF_IsModePowerDown(void);

/**
 * @return NRF in TX mode
 */
uint8_t NRF_IsModeTX(void);

/**
 * @return NRF in RX mode
 */
uint8_t NRF_IsModeRx(void);

/**
 * Add a pipe to the next free slot
 *
 * @param AddrByte - address byte of the peer
 * @param[out] PipeNum - pipe number is written here
 * @return success
 */
bool NRF_AddPipe(uint8_t AddrByte, uint8_t *PipeNum);

/**
 * Convert pipe number to address byte
 *
 * @param pipe_num - pipe number
 * @return address byte
 */
uint8_t NRF_PipeNum2Addr(uint8_t pipe_num);

/**
 * Convert address to a pipe number
 *
 * @param addr
 * @return
 */
uint8_t NRF_Addr2PipeNum(uint8_t addr);

/**
 * Send a packet (takes care of mode switching etc)
 *
 * @param PipeNum - pipe number
 * @param Packet - packet bytes
 * @param Length - packet length
 * @return success (ACK'd)
 */
bool NRF_SendPacket(uint8_t PipeNum, const uint8_t *Packet, uint8_t Length);

/**
 * Receive a packet
 *
 * @param[out] Packet - the receiuved packet - buffer that is written
 * @param[out] PipeNum - pipe number that was received from
 * @return packet size, 0 on failure
 */
uint8_t NRF_ReceivePacket(uint8_t *Packet, uint8_t *PipeNum);

/**
 * Set base address
 * @param Bytes4
 */
void NRF_SetBaseAddress(const uint8_t *Bytes4);

/**
 * Check if there's a packet to be read
 *
 * @return 1 if any to read
 */
bool NRF_IsRxPacket(void);

/**
 * Enable a pipe
 *
 * @param pipenum - pipe number 0-5
 */
void NRF_EnablePipe(uint8_t pipenum);

/**
 * Disable a pipe
 *
 * @param pipenum - pipe number 0-5
 */
void NRF_DisablePipe(uint8_t pipenum);

#endif // SUPPORT_NRF

#endif //GEX_NRF_NRF_H
