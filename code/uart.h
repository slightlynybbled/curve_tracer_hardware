#ifndef _UART_H
#define _UART_H

#include <stdint.h>

#define TX_BUF_LENGTH       128
#define RX_BUF_LENGTH       32

void UART_init(void);

void UART_read(void* data, uint32_t length);
void UART_write(void* data, uint32_t length);
int16_t UART_readable(void);
int16_t UART_writeable(void);

#endif

