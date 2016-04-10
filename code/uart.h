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
void UART_read(void* data, uint32_t length);

/**
 * Writes data to the UART send circular buffer
 * 
 * @param data source array pointer of the data to write
 * @param length length of the data to write
 */
void UART_write(void* data, uint32_t length);

/**
 * Returns the number of bytes waiting to be read
 * from the UART circuilar buffer
 * 
 * @return length of the data
 */
int16_t UART_readable(void);

/**
 * Returns the number of bytes that can be written
 * @return the number of bytes that can be written
 */
int16_t UART_writeable(void);

#endif

