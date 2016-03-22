#ifndef _UART_H
#define _UART_H

#include <stdint.h>



void UART_init(void);

int32_t UART_read(uint8_t* data, uint32_t length);
int32_t UART_write(uint8_t* data, uint32_t length);
int32_t UART_readable(void);
int32_t UART_writeable(void);


#endif

