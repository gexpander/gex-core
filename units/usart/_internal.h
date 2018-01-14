//
// Created by MightyPork on 2018/01/14.
//

#ifndef GEX_F072_UUSART_INTERNAL_H
#define GEX_F072_UUSART_INTERNAL_H

#ifndef UUSART_INTERNAL
#error "Bad include"
#endif

#include "platform.h"

#define UUSART_RXBUF_LEN 128
#define UUSART_TXBUF_LEN 128

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

    bool data_inv;  //!< Invert data bytes
    bool rx_inv;    //!< Invert the RX pin levels
    bool tx_inv;    //!< Invert the TX pin levels

    bool de_output; //!< Generate the Driver Enable signal for RS485
    bool de_polarity; //!< DE active level
    uint8_t de_assert_time;   //!< Time to assert the DE signal before transmit
    uint8_t de_clear_time; //!< Time to clear the DE signal after transmit

    USART_TypeDef *periph;

    DMA_TypeDef *dma;
    DMA_Channel_TypeDef *dma_rx;
    DMA_Channel_TypeDef *dma_tx;
    uint8_t dma_rx_chnum;
    uint8_t dma_tx_chnum;

    uint8_t *rx_buffer;
    uint8_t *tx_buffer;
    uint16_t rx_buf_readpos;
};

#endif //GEX_F072_UUSART_INTERNAL_H
