#include "dio.h"
#include <xc.h>

#define DIO_OUTPUT  0
#define DIO_INPUT   1
#define DIO_DIGITAL 0
#define DIO_ANALOG  1

void DIO_makeInput(Port port, uint8_t pin){
    uint16_t mask = 1 << pin;
    
    switch(port){
        case DIO_PORT_A:
        {
            TRISA |= mask;
            break;
        }
        
        case DIO_PORT_B:
        {
            TRISB |= mask;
            break;
        }
        
        default: while(1);
    }
}

void DIO_makeOutput(Port port, uint8_t pin){
    uint16_t mask = ~(1 << pin);
    
    switch(port){
        case DIO_PORT_A:
        {
            TRISA &= mask;
            break;
        }
        
        case DIO_PORT_B:
        {
            TRISB &= mask;
            break;
        }
        
        default: while(1);
    }
}

void DIO_makeAnalog(Port port, uint8_t pin){
    uint16_t mask = 1 << pin;
    
    switch(port){
        case DIO_PORT_A:
        {
            ANSA |= mask;
            break;
        }
        
        case DIO_PORT_B:
        {
            ANSB |= mask;
            break;
        }
        
        default: while(1);
    }
}

void DIO_makeDigital(Port port, uint8_t pin){
    uint16_t mask = ~(1 << pin);
    
    switch(port){
        case DIO_PORT_A:
        {
            ANSA &= mask;
            break;
        }
        
        case DIO_PORT_B:
        {
            ANSB &= mask;
            break;
        }
        
        default: while(1);
    }
}