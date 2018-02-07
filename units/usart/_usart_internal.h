//
// Created by MightyPork on 2018/01/14.
//
// Internal defines and consts used by the USART unit.
// Can be included only by the USART c files.
//

#ifndef GEX_F072_UUSART_INTERNAL_H
#define GEX_F072_UUSART_INTERNAL_H

#ifndef UUSART_INTERNAL
#error "Bad include"
#endif

#include "platform.h"

#define UUSART_RXBUF_LEN 128
#define UUSART_TXBUF_LEN 128

#define UUSART_DIRECTION_RX 1
#define UUSART_DIRECTION_TX 2
#define UUSART_DIRECTION_RXTX 3

/** Private data structure */
struct priv {
    uint8_t periph_num; //!< 1-6
    uint8_t remap;      //!< UART remap option

    uint32_t baudrate;  //!< baud rate
    uint8_t parity;     //!< 0-none, 1-odd, 2-even
    uint8_t stopbits;   //!< 0-half, 1-one, 2-one-and-half, 3-two (halves - 1)
    uint8_t direction;  //!< 1-RX, 2-TX, 3-RXTX

    uint8_t hw_flow_control; //!< HW flow control 0-none, 1-RTC, 2-CTS, 3-full
    bool clock_output;  //!< Output serial clock
    bool cpol;          //!< clock CPOL setting
    bool cpha;          //!< clock CPHA setting
    bool lsb_first;     //!< bit order
    uint8_t width;      //!< word width - 7, 8, 9 (this includes parity)

    bool data_inv;      //!< Invert data bytes
    bool rx_inv;        //!< Invert the RX pin levels
    bool tx_inv;        //!< Invert the TX pin levels

    bool de_output;   //!< Generate the Driver Enable signal for RS485
    bool de_polarity; //!< DE active level
    uint8_t de_assert_time; //!< Time to assert the DE signal before transmit
    uint8_t de_clear_time;  //!< Time to clear the DE signal after transmit

    USART_TypeDef *periph; //!<  USART peripheral

    DMA_TypeDef *dma;      //!< DMA peripheral
    uint8_t dma_rx_chnum; //!< DMA rx channel number (resolved dynamically based on availability)
    uint8_t dma_tx_chnum; //!< DMA tx channel number (resolved dynamically based on availability)
    DMA_Channel_TypeDef *dma_rx; //!< DMA rx channel instance
    DMA_Channel_TypeDef *dma_tx; //!< DMA tx channel instance

    // DMA stuff
    volatile uint8_t *rx_buffer;      //!< Receive buffer (malloc'd). Has configured TC and TH interrupts.
    volatile uint16_t rx_buf_readpos; //!< Start of the next read (sending to USB)
    volatile uint16_t rx_last_dmapos; //!< Last position of the DMA cyclic write. Used to detect timeouts and for partial data capture from the buffer

    volatile uint8_t *tx_buffer;    //!< Transmit buffer (malloc'd)
    volatile uint16_t tx_buf_nr;    //!< Next Read index
    volatile uint16_t tx_buf_nw;    //!< Next Write index
    volatile uint16_t tx_buf_chunk; //!< Size of the currently being transmitted chunk (for advancing the pointers)
    volatile bool tx_dma_busy;      //!< Flag that the Tx DMA request is ongoing
};

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
void UUSART_loadBinary(Unit *unit, PayloadParser *pp);

/** Write to a binary buffer for storing in Flash */
void UUSART_writeBinary(Unit *unit, PayloadBuilder *pb);

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
error_t UUSART_loadIni(Unit *unit, const char *key, const char *value);

/** Generate INI file section for the unit */
void UUSART_writeIni(Unit *unit, IniWriter *iw);

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
error_t UUSART_preInit(Unit *unit);

/** Tear down the unit */
void UUSART_deInit(Unit *unit);

/** Finalize unit set-up */
error_t UUSART_init(Unit *unit);

/**
 * Handle received data (we're inside the IRQ)
 *
 * @param unit - handled unit
 * @param endpos - end position in the buffer
 */
void UUSART_DMA_HandleRxFromIRQ(Unit *unit, uint16_t endpos);

/**
 * Put data on the queue. Only a part may be sent due to a buffer size limit.
 *
 * @param priv
 * @param buffer - buffer to send
 * @param len - buffer size
 * @return number of bytes that were really written (from the beginning)
 */
uint16_t UUSART_DMA_TxQueue(struct priv *priv, const uint8_t *buffer, uint16_t len);

// ------------------------------------------------------------------------


#endif //GEX_F072_UUSART_INTERNAL_H
