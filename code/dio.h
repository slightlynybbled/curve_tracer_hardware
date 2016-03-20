#ifndef _DIO_H
#define _DIO_H

#include <stdint.h>

typedef enum port{DIO_PORT_A, DIO_PORT_B}Port;



void DIO_makeInput(Port port, uint8_t pin);
void DIO_makeOutput(Port port, uint8_t pin);
void DIO_makeAnalog(Port port, uint8_t pin);
void DIO_makeDigital(Port port, uint8_t pin);


#endif
