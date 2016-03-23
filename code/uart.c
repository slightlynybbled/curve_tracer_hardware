#include "uart.h"
#include "cbuffer.h"
#include <xc.h>

volatile static Buffer txBuf;
volatile static Buffer rxBuf;

void UART_init(void){
    ANSBbits.ANSB2 = ANSBbits.ANSB7 = 0;
    
    BUF_init((Buffer*)&txBuf);
    BUF_init((Buffer*)&rxBuf);
    
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

int32_t UART_read(void* data, uint32_t length){
    uint32_t i = 0;
    uint8_t* d = (uint8_t*)data;
    
    while(i < length){
        d[i] = BUF_read((Buffer*)&rxBuf);
        i++;
    }
    
    return 1;
}

int32_t UART_write(void* data, uint32_t length){
    uint32_t i = 0;
    uint8_t* d = (uint8_t*)data;
    
    while(i < length){
        BUF_write((Buffer*)&txBuf, d[i]);
        i++;
    }
    
    /* if the transmit isn't active, then kick-start the
     * transmit; the interrupt routine will finish sending
     * the remainder of the buffer */
    if(U1STAbits.TRMT == 1){
        U1TXREG = BUF_read((Buffer*)&txBuf);
    }
    
    return 1;
}

int32_t UART_readable(void){
    return (int32_t)BUF_fullSlots((Buffer*)&rxBuf);
}

int32_t UART_writeable(void){
    return (int32_t)BUF_emptySlots((Buffer*)&txBuf);
}

void _ISR _U1TXInterrupt(void){
    /* read the byte(s) to be transmitted from the tx circular
     * buffer and transmit using the hardware register */
    while((BUF_status((Buffer*)&txBuf) != BUFFER_EMPTY)
            && (U1STAbits.UTXBF == 0)){
        
        U1TXREG = BUF_read((Buffer*)&txBuf);
    }
    
    IFS0bits.U1TXIF = 0;
}

void _ISR _U1RXInterrupt(void){
    /* read the received byte(s) from the register and write
     * to the rx circular buffer */
    while((BUF_status((Buffer*)&rxBuf) != BUFFER_FULL)
            && (U1STAbits.URXDA)){
        BUF_write((Buffer*)&rxBuf, U1RXREG);
    }
    
    
    IFS0bits.U1RXIF = 0;
}


