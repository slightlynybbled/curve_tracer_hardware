#include "uart.h"
#include "dio.h"
#include <xc.h>

void sendByte(char byte);

void initUart(void){
    DIO_makeDigital(DIO_PORT_B, 2);
    DIO_makeDigital(DIO_PORT_B, 7);
    
    /* baud rate = 57600bps 
     * U1BRG = (12000000/(16*57600)) - 1 = 12.02 = 12
     */
    U1BRG = 12;
    U1MODE = 0x0000;    // TX/RX only, standard mode
    U1STA = 0x0000;     // enable TX
    
    /* uart interrupts */
    IFS0bits.U1TXIF = IFS0bits.U1RXIF = 0;
    IEC0bits.U1TXIE = IEC0bits.U1RXIE = 1;
    
    U1MODEbits.UARTEN = 1;
    U1STAbits.UTXEN = 1;
    
    return;
}

void sendByte(char byte){
    /* wait for buffers to clear */
    while(U1STAbits.UTXBF == 1);
    
    U1TXREG = byte;
}

void _ISR _U1TXInterrupt(void){
    
    IFS0bits.U1TXIF = 0;
}

void _ISR _U1RXInterrupt(void){
    U1TXREG = U1RXREG;  // echo
    
    IFS0bits.U1RXIF = 0;
}
