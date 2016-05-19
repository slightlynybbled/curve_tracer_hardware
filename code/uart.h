#ifndef _UART_H
#define _UART_H

#include <stdint.h>

#define TX_BUF_LENGTH       256
#define RX_BUF_LENGTH       32

/**
 * Initializes the UART
 */
void UART_init(void);

/**
 * Reads data from the UART receive circular buffer
 * 
 * @param data destination array pointer
 * @param length number of bytes to read
 */
void UART_read(uint8_t* data, uint16_t length);

/**
 * Writes data to the UART send circular buffer
 * 
 * @param data source array pointer of the data to write
 * @param length length of the data to write
 */
void UART_write(uint8_t* data, uint16_t length);

/**
 * Returns the number of bytes waiting to be read
 * from the UART circuilar buffer
 * 
 * @return length of the data
 */
uint16_t UART_readable(void);

/**
 * Returns the number of bytes that can be written
 * @return the number of bytes that can be written
 */
uint16_t UART_writeable(void);

#if (TX_BUF_LENGTH != 2) && \
    (TX_BUF_LENGTH != 4) && \
    (TX_BUF_LENGTH != 8) && \
    (TX_BUF_LENGTH != 16) && \
    (TX_BUF_LENGTH != 32) && \
    (TX_BUF_LENGTH != 64) && \
    (TX_BUF_LENGTH != 128) && \
    (TX_BUF_LENGTH != 256) && \
    (TX_BUF_LENGTH != 512) && \
    (TX_BUF_LENGTH != 1024) && \
    (TX_BUF_LENGTH != 2048)
#error "TX_BUF_LENGTH must be a power of 2"
#endif

#if (RX_BUF_LENGTH != 2) && \
    (RX_BUF_LENGTH != 4) && \
    (RX_BUF_LENGTH != 8) && \
    (RX_BUF_LENGTH != 16) && \
    (RX_BUF_LENGTH != 32) && \
    (RX_BUF_LENGTH != 64) && \
    (RX_BUF_LENGTH != 128) && \
    (RX_BUF_LENGTH != 256) && \
    (RX_BUF_LENGTH != 512) && \
    (RX_BUF_LENGTH != 1024) && \
    (RX_BUF_LENGTH != 2048)
#error "RX_BUF_LENGTH must be a power of 2"
#endif

#endif

