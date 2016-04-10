#include "frame.h"
#include "uart.h"
#include <stddef.h>

#define SOF 0xf7
#define EOF 0x7f
#define ESC 0xf6
#define ESC_XOR 0x20

static uint8_t rxFrame[RX_FRAME_LENGTH];
static uint16_t rxFrameIndex = 0;

void FRM_pushByte(uint8_t data);
uint16_t FRM_fletcher16(uint8_t* data, size_t bytes);

void FRM_init(void){
    UART_init();
}

void FRM_push(uint8_t* data, uint16_t length){
    int16_t i = 0;
    uint8_t dataToSend;
    
    uint16_t check = FRM_fletcher16(data, length);
    uint8_t check0 = (uint8_t)(check & 0x00ff);
    uint8_t check1 = (uint8_t)((check & 0xff00) >> 8);
    
    /* wait for send buffer to clear, then write the SOF */
    while(UART_writeable() == 0);
    dataToSend = SOF;
    UART_write(&dataToSend, 1);
    
    /* copy the data from data to the txFrame */
    while(i < length){
        FRM_pushByte(data[i++]);
    }
    
    FRM_pushByte(check0);
    FRM_pushByte(check1);
    
    /* wait for send buffer to clear, then write the SOF */
    while(UART_writeable() == 0);
    dataToSend = EOF;
    UART_write(&dataToSend, 1);
}

void FRM_pushByte(uint8_t data){
    /* ensure that at least two bytes can be written */
    while(UART_writeable() < 2);
    
    /* add proper escape sequences */
    if((data == SOF) || (data == EOF) || (data == ESC)){
        uint8_t escChar = ESC;
        uint8_t escData = data ^ ESC_XOR;
        UART_write(&escChar, 1);
        UART_write(&escData, 1);
    }else{
        UART_write(&data, 1);
    }
}

uint16_t FRM_pull(uint8_t* data){
    uint16_t sofIndex = 0, eofIndex = 0;
    uint16_t length = 0;
    
    uint16_t i = 0;
    
    /* read the available bytes into the rxFrame */
    uint16_t numOfBytes = UART_readable();
    UART_read(&rxFrame[rxFrameIndex], numOfBytes);
    rxFrameIndex += numOfBytes;
    
    /* find the SOF */
    while(i < RX_FRAME_LENGTH){
        if(rxFrame[i] == SOF){
            sofIndex = i;
            break;
        }else{
            rxFrame[i] = 0; // clear the byte that isn't the SOF
        }
        i++;
    }
    
    /* find the EOF */
    i = sofIndex;
    while(i < RX_FRAME_LENGTH){
        if(rxFrame[i] == EOF){
            eofIndex = i;
            break;
        }
        i++;
    }
    
    /* ensure that the start of frame and the end of frame are both present */
    if(((rxFrame[0] == SOF) || (sofIndex > 0)) && (eofIndex != 0)){
        /* extract the received frame and shift the remainder of the
         * bytes into the beginning of the frame */
        i = 0;
        int16_t frameIndex = sofIndex;
        rxFrame[frameIndex] = 0;
        frameIndex++;
        while(frameIndex < eofIndex){
            if(rxFrame[frameIndex] == ESC){
                frameIndex++;
                data[i] = rxFrame[frameIndex] ^ ESC_XOR;
            }else{
                data[i] = rxFrame[frameIndex];
            }
            i++;
            frameIndex++;
        }
        
        length = i;
        
        /* a full frame was just processed, find the next SOF and
         * copy the remainder of the frame forward in preparation for
         * the next frame reception */
        i = eofIndex;
        sofIndex = 0;
        while(i < RX_FRAME_LENGTH){
            if(rxFrame[i] == SOF){
                sofIndex = i;
                break;
            }else{
                rxFrame[i] = 0;
            }
            i++;
        }
        rxFrameIndex = 0;
        
        /* copy and clear */
        if(sofIndex > 0){
            i = 0;
            while((i + sofIndex) < RX_FRAME_LENGTH){
                rxFrame[i] = rxFrame[(i + sofIndex)];
                rxFrame[(i + sofIndex)] = 0;
                i++;
            }
        }
        
        /* check the data integrity using the last two bytes as
         * the fletcher16 checksum */
        uint16_t checksum = data[length - 2] | (data[length - 1] << 8);
        length -= 2;
        uint16_t check = FRM_fletcher16(data, length);
        if(check != checksum)
            length = 0;
    }
    
    return length;
}

uint16_t FRM_fletcher16(uint8_t* data, size_t length){
	uint16_t sum1 = 0x00ff;
	uint16_t sum2 = 0x00ff;
    
    uint16_t i = 0;
    while(i < length){
        sum1 += (uint16_t)data[i];
        sum2 += sum1;
        i++;
    }
    
	sum1 = (sum1 & 0x00ff) + (sum1 >> 8);
	sum2 = (sum2 & 0x00ff) + (sum2 >> 8);
    
    uint16_t checksum = (sum2 << 8) | sum1;
    
	return checksum;
}

