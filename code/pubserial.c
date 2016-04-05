#include "pubserial.h"
#include "frame.h"
#include "cbuffer.h"
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_NUM_OF_FORMAT_SPECIFIERS    4
#define MAX_TRANSMIT_MESSAGE_LEN        128

typedef enum formatspecifier{
    eNONE = 0,
    eSTRING = 1,
    eU8 = 2,
    eS8 = 3,
    eU16 = 4,
    eS16 = 5,
    eU32 = 6,
    eS32 = 7,
    eFLOAT = 8
}FormatSpecifier;

typedef struct{
    uint8_t dimensions;
    uint16_t length;
    uint32_t length8bit;
    uint8_t formatSpecifiers[MAX_NUM_OF_FORMAT_SPECIFIERS];
    
    uint8_t data[MAX_TRANSMIT_MESSAGE_LEN];
}Message;

/********** global variable declarations **********/
static Message msg;

/********** local function declarations **********/
void publish_str(const char* t, const char* msg);
void publish_u8(const char* t, uint8_t* data, uint16_t dataLength);
void publish_s8(const char* t, int8_t* data, uint16_t dataLength);
void publish_u16(const char* t, uint16_t* data, uint16_t dataLength);
void publish_s16(const char* t, int16_t* data, uint16_t dataLength);
void publish_u32(const char* t, uint32_t* data, uint16_t dataLength);
void publish_s32(const char* t, int32_t* data, uint16_t dataLength);

uint16_t getCurrentMessageWidth(void);

/********** function implementations **********/
void pubserial_init(void){
    FRM_init();
}

void publish(const char* t, ...){
    va_list arguments;
    va_start(arguments, t);
    
    /* initialize the message */
    msg.dimensions = 0;
    msg.length = 0;
    msg.length8bit = 0;
    
    uint16_t i;
    for(i = 0; i < MAX_NUM_OF_FORMAT_SPECIFIERS; i++){
        msg.formatSpecifiers[i] = 0;
    }
    
    /* get the num of args by counting the commas */
    uint8_t commaCount = 0;
    uint16_t len = strlen(t);
    i = 0;
    while(i < len){
        if(t[i] == ',')
            commaCount++;
        i++;
    }
    
    /* go through the first argument and extract the topic */
    uint16_t strIndex = 0;
    char topic[16] = {0};
    while((strIndex < len) && (t[strIndex] != ',') && (t[strIndex] != ':')){
        topic[strIndex] = t[strIndex];        
        strIndex++;
    }
    
    /* determine if there is more of the string left to process and, if
     * there is, then process the index */
    msg.length = 0;
    if(strIndex < len){
        /* check for the ':' character to know if this 
         * is array or single-point processing */
        if(t[strIndex] == ':'){
            strIndex++;

            /* find he first number to the colon command or
             * the end of the string */
            char strNum0[8] = {0};
            i = 0;
            while((t[strIndex] != ',') && (t[strIndex] != 0)){
                strNum0[i] = t[strIndex];
                i++;
                strIndex++;
            }
            /* convert the ASCII number to an 
             * integer and save it in arrIndex0 */
            msg.length = (uint16_t)atol(strNum0);
        }
    }
    
    /* place a minimum on the arrIndex */
    if(msg.length < 1)
        msg.length = 1;
    
    /* check the format specifiers */
    i = 0;
    while((strIndex < len) && (i < MAX_NUM_OF_FORMAT_SPECIFIERS)){
        if(t[strIndex] == ','){
            strIndex++;
            if(t[strIndex] == 'u'){
                strIndex++;
                if(t[strIndex] == '8'){
                    msg.formatSpecifiers[i] = eU8;
                }else if(t[strIndex] == '1'){
                    /* if the first digit is '1', then the next digit must
                     * be 6, so there is no need to check for it */
                    msg.formatSpecifiers[i] = eU16;
                    strIndex++;
                }else if(t[strIndex] == '3'){
                    /* if the first digit is '3', then the next digit must
                     * be 2, so there is no need to check for it */
                    msg.formatSpecifiers[i] = eU32;
                    strIndex++;
                }
            }else if(t[strIndex] == 's'){
                strIndex++;
                if(t[strIndex] == '8'){
                    msg.formatSpecifiers[i] = eS8;
                }else if(t[strIndex] == '1'){
                    /* if the first digit is '1', then the next digit must
                     * be 6, so there is no need to check for it */
                    msg.formatSpecifiers[i] = eS16;
                    strIndex++;
                }else if(t[strIndex] == '3'){
                    /* if the first digit is '3', then the next digit must
                     * be 2, so there is no need to check for it */
                    msg.formatSpecifiers[i] = eS32;
                    strIndex++;
                }else if(t[strIndex] == 't'){
                    /* this is the case which calls for a string to be sent */
                    msg.formatSpecifiers[i] = eSTRING;
                    strIndex++;
                }
            }else if(t[strIndex] == 'f'){
                msg.formatSpecifiers[i] = eFLOAT;
            }
            strIndex++;
        }else{
            /* if a comma isn't here, then abort the format specifier */
            break;
        }
        
        i++;
    }
    
    /* at this point:
     *     1. topic stored in topic[]
     *     2. array indexes are in arrIndex0 and arrIndex1
     *     3. format specifiers are in msg.formatSpecifiers[] array */
    i = 0;
    do{
        switch(msg.formatSpecifiers[i]){
            /* no format specifiers means U8 */
            case eNONE:
            case eU8:
            {
                uint8_t* data = va_arg(arguments, uint8_t*);
                publish_u8(topic, data, msg.length);
                break;
            }
            
            case eS8:
            {
                int8_t* data = va_arg(arguments, int8_t*);
                publish_s8(topic, data, msg.length);
                break;
            }
            
            case eU16:
            {
                uint16_t* data = va_arg(arguments, uint16_t*);
                publish_u16(topic, data, msg.length);
                break;
            }
            
            case eS16:
            {
                int16_t* data = va_arg(arguments, int16_t*);
                publish_s16(topic, data, msg.length);
                break;
            }
            
            case eU32:
            {
                uint32_t* data = va_arg(arguments, uint32_t*);
                publish_u32(topic, data, msg.length);
                break;
            }
            
            case eS32:
            {
                int32_t* data = va_arg(arguments, int32_t*);
                publish_s32(topic, data, msg.length);
                break;
            }
            
            case eFLOAT:
            {
                
                break;
            }
            
            case eSTRING:
            {
                //publish_str(topic, arg0);
                
                i = commaCount;
                break;
            }
            

        }
        
        i++;
    }while(i < commaCount);
    
    /* format the message structure into its final form before
     * sending to the framing engine */
    uint8_t msgData[MAX_TRANSMIT_MESSAGE_LEN] = {0};
    uint16_t msgDataIndex = 0;
    
    /* copy the topic */
    i = 0;
    while(topic[i] != 0){
        msgData[msgDataIndex] = topic[i];
        msgDataIndex++;
        i++;
    }
    msgDataIndex++; // place a '\0' to delimit the topic string
    
    /* append the dimensions and length */
    msgData[msgDataIndex++] = msg.dimensions;
    msgData[msgDataIndex++] = (uint8_t)(msg.length & 0x00ff);
    msgData[msgDataIndex++] = (uint8_t)((msg.length & 0xff00) >> 8);
    
    /* append all format specifiers */
    i = 0;
    while(i < msg.dimensions){
        if((i & 1) == 0){
            msgData[msgDataIndex] = msg.formatSpecifiers[i] & 0x0f;
        }else{
            msgData[msgDataIndex++] |= ((msg.formatSpecifiers[i] & 0x0f) << 4);
        }
        
        i++;
    }
    
    /* if the previous step results in an odd number being written, then
     * ensure that the msgDataIndex is incremented so that the data doesn't
     * get overwritten */
    if(i & 1){
        msgDataIndex++;
    }
    
    /* append the data */
    i = 0;
    while(i < msg.length8bit){
        msgData[msgDataIndex++] = msg.data[i++];
    }
    
    /* send the data */
    FRM_push(msgData, msg.length);
}

void publish_str(const char* t, const char* textMsg){
    uint16_t i = 0, j = 0;
    
    /* copy the topic string */
    while(t[i] != 0){
        msg.data[i] = t[i];
        i++;
    }
    
    i++;
    
    /* place the dimension, length, and format specifier */
    /* dimension is the lower 4 bits after the topic '\0'
     *      the upper 4 bits are reserved
     * length is the next 16 bits
     * format specifier is the lower 4 bits of the next byte */
    msg.data[i++] = 0;     // dimension = 0 on string
    
    msg.data[i++] = 0;     // length is until '\0' on a string, so this is 0 as well
    msg.data[i++] = 0;
    
    msg.data[i++] = eSTRING;   // format specifier is 'string'    
    
    /* copy the string itself */
    while(textMsg[j] != 0){
        msg.data[i] = textMsg[j];
        j++;
        i++;
    }
}

void publish_u8(const char* t, uint8_t* data, uint16_t dataLength){
    /* need to find a way to 'pass forward' the message so that it can
     * be amended by subsequent passes through this or another publish function */
    
    /* find the next index to be written by finding the dimensions 
     * and the length of previous entries into msg */
    uint16_t i = 0;
    uint16_t currentIndex = getCurrentMessageWidth();

    msg.formatSpecifiers[msg.dimensions] = eU8;
    msg.dimensions++;
    msg.length8bit += dataLength;
    
    for(i = 0; i < dataLength; i++){
        msg.data[i + currentIndex] = data[i];
    }
}

void publish_s8(const char* t, int8_t* data, uint16_t dataLength){
    /* need to find a way to 'pass forward' the message so that it can
     * be amended by subsequent passes through this or another publish function */
    
    /* find the next index to be written by finding the dimensions 
     * and the length of previous entries into msg */
    uint16_t i = 0;
    uint16_t currentIndex = getCurrentMessageWidth();
    
    msg.formatSpecifiers[msg.dimensions] = eS8;
    msg.dimensions++;
    msg.length8bit += dataLength;
    
    for(i = 0; i < dataLength; i++){
        msg.data[i + currentIndex] = (uint8_t)data[i];
    }
}

void publish_u16(const char* t, uint16_t* data, uint16_t dataLength){
    /* need to find a way to 'pass forward' the message so that it can
     * be amended by subsequent passes through this or another publish function */
    
    /* find the next index to be written by finding the dimensions 
     * and the length of previous entries into msg */
    uint16_t i = 0;
    uint16_t currentIndex = getCurrentMessageWidth();
    
    msg.formatSpecifiers[msg.dimensions] = eU16;
    msg.dimensions++;
    msg.length8bit += (dataLength << 1);
    
    i = 0;
    uint16_t dataLength8bit = dataLength << 1;
    while(i < dataLength8bit){
        uint16_t dataIndex = i >> 1;
        
        msg.data[i + currentIndex] = (uint8_t)(data[dataIndex] & 0x00ff);
        i++;
        msg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0xff00) >> 8);
        i++;
    }
}

void publish_s16(const char* t, int16_t* data, uint16_t dataLength){
    /* need to find a way to 'pass forward' the message so that it can
     * be amended by subsequent passes through this or another publish function */
    
    /* find the next index to be written by finding the dimensions 
     * and the length of previous entries into msg */
    uint16_t i = 0;
    uint16_t currentIndex = getCurrentMessageWidth();
    
    msg.formatSpecifiers[msg.dimensions] = eS16;
    msg.dimensions++;
    msg.length8bit += (dataLength << 1);
    
    i = 0;
    uint16_t dataLength8bit = dataLength << 1;
    while(i < dataLength8bit){
        uint16_t dataIndex = i >> 1;
        
        msg.data[i + currentIndex] = (uint8_t)(data[dataIndex] & 0x00ff);
        i++;
        msg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0xff00) >> 8);
        i++;
    }
}

void publish_u32(const char* t, uint32_t* data, uint16_t dataLength){
    /* need to find a way to 'pass forward' the message so that it can
     * be amended by subsequent passes through this or another publish function */
    
    /* find the next index to be written by finding the dimensions 
     * and the length of previous entries into msg */
    uint16_t i = 0;
    uint16_t currentIndex = getCurrentMessageWidth();
    
    msg.formatSpecifiers[msg.dimensions] = eU32;
    msg.dimensions++;
    msg.length8bit += (dataLength << 2);
    
    i = 0;
    uint16_t dataLength8bit = dataLength << 2;
    while(i < dataLength8bit){
        uint16_t dataIndex = i >> 2;
        
        msg.data[i + currentIndex] = (uint8_t)(data[dataIndex] & 0x000000ff);
        i++;
        msg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0x0000ff00) >> 8);
        i++;
        msg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0x00ff0000) >> 16);
        i++;
        msg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0xff000000) >> 24);
        i++;
    }
}

void publish_s32(const char* t, int32_t* data, uint16_t dataLength){
    /* need to find a way to 'pass forward' the message so that it can
     * be amended by subsequent passes through this or another publish function */
    
    /* find the next index to be written by finding the dimensions 
     * and the length of previous entries into msg */
    uint16_t i = 0;
    uint16_t currentIndex = getCurrentMessageWidth();
    
    msg.formatSpecifiers[msg.dimensions] = eS32;
    msg.dimensions++;
    msg.length8bit += (dataLength << 2);
    
    i = 0;
    uint16_t dataLength8bit = dataLength << 2;
    while(i < dataLength8bit){
        uint16_t dataIndex = i >> 2;
        
        msg.data[i + currentIndex] = (uint8_t)(data[dataIndex] & 0x000000ff);
        i++;
        msg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0x0000ff00) >> 8);
        i++;
        msg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0x00ff0000) >> 16);
        i++;
        msg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0xff000000) >> 24);
        i++;
    }
}

uint16_t getCurrentMessageWidth(void){
    uint16_t i = 0;
    uint16_t currentIndex = 0;
    for(i = 0; i < msg.dimensions; i++){
        uint16_t widthInBytes = 0;
        if((msg.formatSpecifiers[i] == eNONE)
                || (msg.formatSpecifiers[i] == eU8)
                || (msg.formatSpecifiers[i] == eS8)){
            widthInBytes = 1;
        }else if((msg.formatSpecifiers[i] == eU16)
                || (msg.formatSpecifiers[i] == eS16)){
            widthInBytes = 2;
        }else{
            widthInBytes = 4;
        }
        
        currentIndex += msg.length * msg.dimensions * widthInBytes;
    }
    
    return currentIndex;
}
