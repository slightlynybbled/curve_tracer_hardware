#include "pubserial.h"
#include "frame.h"
#include "cbuffer.h"
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_NUM_OF_FORMAT_SPECIFIERS    4
#define MAX_NUM_OF_SUBSCRIPTIONS        4
#define MAX_TOPIC_STR_LEN               16
#define MAX_TRANSMIT_MESSAGE_LEN        64
#define MAX_RECEIVE_MESSAGE_LEN         32

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
    FormatSpecifier formatSpecifiers[MAX_NUM_OF_FORMAT_SPECIFIERS];
    
    uint8_t data[MAX_TRANSMIT_MESSAGE_LEN];
}Message;

typedef struct {
    char topic[MAX_TOPIC_STR_LEN];
	void (*subFunctPtr)();
}Subscription;

/********** global variable declarations **********/
static Message txMsg;
static Message rxMsg;
static Subscription sub[MAX_NUM_OF_SUBSCRIPTIONS];

/********** local function declarations **********/
void publish_str(const char* msg);
void publish_u8(uint8_t* data, uint16_t dataLength);
void publish_s8(int8_t* data, uint16_t dataLength);
void publish_u16(uint16_t* data, uint16_t dataLength);
void publish_s16(int16_t* data, uint16_t dataLength);
void publish_u32(uint32_t* data, uint16_t dataLength);
void publish_s32(int32_t* data, uint16_t dataLength);

uint16_t getCurrentMessageWidth(void);
uint16_t getCurrentRxPointerIndex(uint8_t element);

/********** function implementations **********/
void PUB_init(void){
    uint16_t i = 0;
    
    /* clear the subscriptions */
    for(i = 0; i < MAX_NUM_OF_SUBSCRIPTIONS; i++){
        sub[i].subFunctPtr = 0;
        sub[i].topic[0] = 0;    // terminate the string
    }
    
    FRM_init();
}

void PUB_publish(const char* topic, ...){
    va_list arguments;
    va_start(arguments, topic);
    
    /* initialize the message */
    txMsg.dimensions = 0;
    txMsg.length = 0;
    txMsg.length8bit = 0;
    
    uint16_t i;
    for(i = 0; i < MAX_NUM_OF_FORMAT_SPECIFIERS; i++){
        txMsg.formatSpecifiers[i] = 0;
    }
    
    /* get the num of args by counting the commas */
    uint8_t commaCount = 0;
    uint16_t len = strlen(topic);
    i = 0;
    while(i < len){
        if(topic[i] == ',')
            commaCount++;
        i++;
    }
    
    /* go through the first argument and extract the topic */
    uint16_t strIndex = 0;
    char topicStr[16] = {0};
    while((strIndex < len) && (topic[strIndex] != ',') && (topic[strIndex] != ':')){
        topicStr[strIndex] = topic[strIndex];        
        strIndex++;
    }
    
    /* determine if there is more of the string left to process and, if
     * there is, then process the index */
    txMsg.length = 0;
    if(strIndex < len){
        /* check for the ':' character to know if this 
         * is array or single-point processing */
        if(topic[strIndex] == ':'){
            strIndex++;

            /* find he first number to the colon command or
             * the end of the string */
            char strNum0[8] = {0};
            i = 0;
            while((topic[strIndex] != ',') && (topic[strIndex] != 0)){
                strNum0[i] = topic[strIndex];
                i++;
                strIndex++;
            }
            /* convert the ASCII number to an 
             * integer and save it in arrIndex0 */
            txMsg.length = (uint16_t)atol(strNum0);
        }
    }
    
    /* place a minimum on the arrIndex */
    if(txMsg.length < 1)
        txMsg.length = 1;
    
    /* check the format specifiers */
    i = 0;
    while((strIndex < len) && (i < MAX_NUM_OF_FORMAT_SPECIFIERS)){
        if(topic[strIndex] == ','){
            strIndex++;
            if(topic[strIndex] == 'u'){
                strIndex++;
                if(topic[strIndex] == '8'){
                    txMsg.formatSpecifiers[i] = eU8;
                }else if(topic[strIndex] == '1'){
                    /* if the first digit is '1', then the next digit must
                     * be 6, so there is no need to check for it */
                    txMsg.formatSpecifiers[i] = eU16;
                    strIndex++;
                }else if(topic[strIndex] == '3'){
                    /* if the first digit is '3', then the next digit must
                     * be 2, so there is no need to check for it */
                    txMsg.formatSpecifiers[i] = eU32;
                    strIndex++;
                }
            }else if(topic[strIndex] == 's'){
                strIndex++;
                if(topic[strIndex] == '8'){
                    txMsg.formatSpecifiers[i] = eS8;
                }else if(topic[strIndex] == '1'){
                    /* if the first digit is '1', then the next digit must
                     * be 6, so there is no need to check for it */
                    txMsg.formatSpecifiers[i] = eS16;
                    strIndex++;
                }else if(topic[strIndex] == '3'){
                    /* if the first digit is '3', then the next digit must
                     * be 2, so there is no need to check for it */
                    txMsg.formatSpecifiers[i] = eS32;
                    strIndex++;
                }else if(topic[strIndex] == 't'){
                    /* this is the case which calls for a string to be sent */
                    txMsg.formatSpecifiers[i] = eSTRING;
                    strIndex++;
                }
            }else if(topic[strIndex] == 'f'){
                txMsg.formatSpecifiers[i] = eFLOAT;
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
     *     2. array length is in msg.length
     *     3. format specifiers are in msg.formatSpecifiers[] array */
    i = 0;
    do{
        switch(txMsg.formatSpecifiers[i]){
            /* no format specifiers means U8 */
            case eNONE:
            case eSTRING:
            {
                char* data = va_arg(arguments, char*);
                publish_str(data);
                
                i = commaCount;     // just in case the user supplied more than
                                    // one format specifier (invalid)
                break;
            }
            
            case eU8:
            {
                uint8_t* data = va_arg(arguments, uint8_t*);
                publish_u8(data, txMsg.length);
                break;
            }
            
            case eS8:
            {
                int8_t* data = va_arg(arguments, int8_t*);
                publish_s8(data, txMsg.length);
                break;
            }
            
            case eU16:
            {
                uint16_t* data = va_arg(arguments, uint16_t*);
                publish_u16(data, txMsg.length);
                break;
            }
            
            case eS16:
            {
                int16_t* data = va_arg(arguments, int16_t*);
                publish_s16(data, txMsg.length);
                break;
            }
            
            case eU32:
            {
                uint32_t* data = va_arg(arguments, uint32_t*);
                publish_u32(data, txMsg.length);
                break;
            }
            
            case eS32:
            {
                int32_t* data = va_arg(arguments, int32_t*);
                publish_s32(data, txMsg.length);
                break;
            }
            
            case eFLOAT:
            {
                
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
    while(topicStr[i] != 0){
        msgData[msgDataIndex] = topicStr[i];
        msgDataIndex++;
        i++;
    }
    msgDataIndex++; // place a '\0' to delimit the topic string
    
    /* append the dimensions and length */
    msgData[msgDataIndex++] = txMsg.dimensions;
    msgData[msgDataIndex++] = (uint8_t)(txMsg.length & 0x00ff);
    msgData[msgDataIndex++] = (uint8_t)((txMsg.length & 0xff00) >> 8);
    
    /* append all format specifiers */
    i = 0;
    while(i < txMsg.dimensions){
        if((i & 1) == 0){
            msgData[msgDataIndex] = txMsg.formatSpecifiers[i] & 0x0f;
        }else{
            msgData[msgDataIndex++] |= ((txMsg.formatSpecifiers[i] & 0x0f) << 4);
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
    while(i < txMsg.length8bit){
        msgData[msgDataIndex++] = txMsg.data[i++];
    }
    
    /* send the data */
    FRM_push(msgData, msgDataIndex);
}

void publish_str(const char* textMsg){
    /* copy the string itself */
    uint16_t i = 0;
    while(textMsg[i] != 0){
        txMsg.data[i] = textMsg[i];
        i++;
    }
    
    txMsg.formatSpecifiers[txMsg.dimensions] = eSTRING;
    txMsg.dimensions = 1;
    txMsg.length = txMsg.length8bit = i;
    
}

void publish_u8(uint8_t* data, uint16_t dataLength){
    /* need to find a way to 'pass forward' the message so that it can
     * be amended by subsequent passes through this or another publish function */
    
    /* find the next index to be written by finding the dimensions 
     * and the length of previous entries into msg */
    uint16_t i = 0;
    uint16_t currentIndex = getCurrentMessageWidth();

    txMsg.formatSpecifiers[txMsg.dimensions] = eU8;
    txMsg.dimensions++;
    txMsg.length8bit += dataLength;
    
    for(i = 0; i < dataLength; i++){
        txMsg.data[i + currentIndex] = data[i];
    }
}

void publish_s8(int8_t* data, uint16_t dataLength){
    /* need to find a way to 'pass forward' the message so that it can
     * be amended by subsequent passes through this or another publish function */
    
    /* find the next index to be written by finding the dimensions 
     * and the length of previous entries into msg */
    uint16_t i = 0;
    uint16_t currentIndex = getCurrentMessageWidth();
    
    txMsg.formatSpecifiers[txMsg.dimensions] = eS8;
    txMsg.dimensions++;
    txMsg.length8bit += dataLength;
    
    for(i = 0; i < dataLength; i++){
        txMsg.data[i + currentIndex] = (uint8_t)data[i];
    }
}

void publish_u16(uint16_t* data, uint16_t dataLength){
    /* need to find a way to 'pass forward' the message so that it can
     * be amended by subsequent passes through this or another publish function */
    
    /* find the next index to be written by finding the dimensions 
     * and the length of previous entries into msg */
    uint16_t i = 0;
    uint16_t currentIndex = getCurrentMessageWidth();
    
    txMsg.formatSpecifiers[txMsg.dimensions] = eU16;
    txMsg.dimensions++;
    txMsg.length8bit += (dataLength << 1);
    
    i = 0;
    uint16_t dataLength8bit = dataLength << 1;
    while(i < dataLength8bit){
        uint16_t dataIndex = i >> 1;
        
        txMsg.data[i + currentIndex] = (uint8_t)(data[dataIndex] & 0x00ff);
        i++;
        txMsg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0xff00) >> 8);
        i++;
    }
}

void publish_s16(int16_t* data, uint16_t dataLength){
    /* need to find a way to 'pass forward' the message so that it can
     * be amended by subsequent passes through this or another publish function */
    
    /* find the next index to be written by finding the dimensions 
     * and the length of previous entries into msg */
    uint16_t i = 0;
    uint16_t currentIndex = getCurrentMessageWidth();
    
    txMsg.formatSpecifiers[txMsg.dimensions] = eS16;
    txMsg.dimensions++;
    txMsg.length8bit += (dataLength << 1);
    
    i = 0;
    uint16_t dataLength8bit = dataLength << 1;
    while(i < dataLength8bit){
        uint16_t dataIndex = i >> 1;
        
        txMsg.data[i + currentIndex] = (uint8_t)(data[dataIndex] & 0x00ff);
        i++;
        txMsg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0xff00) >> 8);
        i++;
    }
}

void publish_u32(uint32_t* data, uint16_t dataLength){
    /* need to find a way to 'pass forward' the message so that it can
     * be amended by subsequent passes through this or another publish function */
    
    /* find the next index to be written by finding the dimensions 
     * and the length of previous entries into msg */
    uint16_t i = 0;
    uint16_t currentIndex = getCurrentMessageWidth();
    
    txMsg.formatSpecifiers[txMsg.dimensions] = eU32;
    txMsg.dimensions++;
    txMsg.length8bit += (dataLength << 2);
    
    i = 0;
    uint16_t dataLength8bit = dataLength << 2;
    while(i < dataLength8bit){
        uint16_t dataIndex = i >> 2;
        
        txMsg.data[i + currentIndex] = (uint8_t)(data[dataIndex] & 0x000000ff);
        i++;
        txMsg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0x0000ff00) >> 8);
        i++;
        txMsg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0x00ff0000) >> 16);
        i++;
        txMsg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0xff000000) >> 24);
        i++;
    }
}

void publish_s32(int32_t* data, uint16_t dataLength){
    /* need to find a way to 'pass forward' the message so that it can
     * be amended by subsequent passes through this or another publish function */
    
    /* find the next index to be written by finding the dimensions 
     * and the length of previous entries into msg */
    uint16_t i = 0;
    uint16_t currentIndex = getCurrentMessageWidth();
    
    txMsg.formatSpecifiers[txMsg.dimensions] = eS32;
    txMsg.dimensions++;
    txMsg.length8bit += (dataLength << 2);
    
    i = 0;
    uint16_t dataLength8bit = dataLength << 2;
    while(i < dataLength8bit){
        uint16_t dataIndex = i >> 2;
        
        txMsg.data[i + currentIndex] = (uint8_t)(data[dataIndex] & 0x000000ff);
        i++;
        txMsg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0x0000ff00) >> 8);
        i++;
        txMsg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0x00ff0000) >> 16);
        i++;
        txMsg.data[i + currentIndex] = (uint8_t)((data[dataIndex] & 0xff000000) >> 24);
        i++;
    }
}

uint16_t getCurrentMessageWidth(void){
    uint16_t i = 0;
    uint16_t currentIndex = 0;
    for(i = 0; i < txMsg.dimensions; i++){
        uint16_t widthInBytes = 0;
        if((txMsg.formatSpecifiers[i] == eNONE)
                || (txMsg.formatSpecifiers[i] == eU8)
                || (txMsg.formatSpecifiers[i] == eS8)){
            widthInBytes = 1;
        }else if((txMsg.formatSpecifiers[i] == eU16)
                || (txMsg.formatSpecifiers[i] == eS16)){
            widthInBytes = 2;
        }else{
            widthInBytes = 4;
        }
        
        currentIndex += txMsg.length * widthInBytes;
    }
    
    return currentIndex;
}

void PUB_subscribe(const char* topic, void (*functPtr)()){
    /* find an empty subscription slot */
    uint16_t i;
    uint16_t found = 0;
    for(i = 0; i < MAX_NUM_OF_SUBSCRIPTIONS; i++){
        if(sub[i].subFunctPtr == 0){
            found = 1;
            break;
        }
    }
    
    /* subscribe to the 'found' slot */
    if(found == 1){
        /* copy the function pointer and the topic */
        sub[i].subFunctPtr = functPtr;
        
        uint16_t j = 0;
        while(topic[j] != 0){
            sub[i].topic[j] = topic[j];
            j++;
        }
    }
}

void PUB_unsubscribe(void (*functPtr)()){
    /* find the required subscription slot */
    uint16_t i;
    for(i = 0; i < MAX_NUM_OF_SUBSCRIPTIONS; i++){
        /* copy the function pointer and the topic */
        if(sub[i].subFunctPtr == functPtr){
            sub[i].subFunctPtr = 0;
            sub[i].topic[0] = 0;
        }
    }
}

void PUB_process(void){
    /* retrieve any messages from the framing buffer 
     * and process them appropriately */
    uint8_t data[MAX_RECEIVE_MESSAGE_LEN];
    if(FRM_pull(data) > 0){
        char topic[MAX_TOPIC_STR_LEN] = {0};
        uint16_t i = 0;
        uint16_t dataIndex = 0;
        
        /* decompose the message into its constituent parts */
        while(data[i] != 0){
            topic[i] = data[i];
            i++;
        }

        dataIndex = i + 1;
        
        rxMsg.dimensions = data[dataIndex++] & 0x0f;
        rxMsg.length = (uint16_t)data[dataIndex++];
        rxMsg.length += ((uint16_t)data[dataIndex++]) << 8;
        
        rxMsg.length8bit = 0;
        
        for(i = 0; i < rxMsg.dimensions; i++){
            if((i & 1) == 0){
                rxMsg.formatSpecifiers[i] = data[dataIndex] & 0x0f;
            }else{
                rxMsg.formatSpecifiers[i] = (data[dataIndex++] & 0xf0) >> 4;
            }
            
            if((rxMsg.formatSpecifiers[i] == eSTRING)
                    || (rxMsg.formatSpecifiers[i] == eU8)
                    || (rxMsg.formatSpecifiers[i] == eS8)){
                rxMsg.length8bit += rxMsg.length;
            }else if((rxMsg.formatSpecifiers[i] == eU16)
                    || (rxMsg.formatSpecifiers[i] == eS16)){
                rxMsg.length8bit += (rxMsg.length << 1);
            }else if((rxMsg.formatSpecifiers[i] == eU32)
                    || (rxMsg.formatSpecifiers[i] == eS32)){
                rxMsg.length8bit += (rxMsg.length << 2);
            }
        }
        
        /* ensure that the data index is incremented
         * when the dimensions are odd */
        if(i & 1){
            dataIndex++;
        }
        
        /* keep from having to re-copy the buffer */
        uint8_t* dataWithOffset = data + dataIndex;
        
        /* copy the data to the rx array */
        for(i = 0; i < rxMsg.length8bit; i++){
            rxMsg.data[i] = dataWithOffset[i];
        }
        
        /* go through the active subscriptions and execute any
         * functions that are subscribed to the received topics */
        for(i = 0; i < MAX_NUM_OF_SUBSCRIPTIONS; i++){
            if(strcmp(topic, sub[i].topic) == 0){
                /* execute the function if it isn't empty */
                if(sub[i].subFunctPtr != 0){
                    sub[i].subFunctPtr();
                }
            }
        }
    }
}

uint16_t PUB_getElements(uint16_t element, void* destArray){
    uint16_t i;
    
    uint16_t currentIndex = getCurrentRxPointerIndex(element);
    
    switch(rxMsg.formatSpecifiers[element]){
        case eNONE:
        case eSTRING:
        {
            // typecast to a char
            char* data = (char*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                data[i] = rxMsg.data[i];
                i++;
            }
            
            break;
        }
        
        case eU8:
        {
            // typecast to a char
            uint8_t* data = (uint8_t*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                data[i] = (uint8_t)rxMsg.data[currentIndex + i];
                i++;
            }
            
            break;
        }
        
        case eS8:
        {
            // typecast to a char
            int8_t* data = (int8_t*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                data[i] = (int8_t)rxMsg.data[currentIndex + i];
                i++;
            }
            
            break;
        }
        
        case eU16:
        {
            // typecast to a unsigned int
            uint16_t* data = (uint16_t*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                uint16_t dataIndex = i >> 1;
                data[dataIndex] = (uint16_t)rxMsg.data[currentIndex + i];
                i++;
                data[dataIndex] |= (uint16_t)rxMsg.data[currentIndex + i] << 8;
                i++;
            }
            
            break;
        }
        
        case eS16:
        {
            // typecast to a int
            int16_t* data = (int16_t*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                uint16_t dataIndex = i >> 1;
                data[dataIndex] = (int16_t)rxMsg.data[currentIndex + i];
                i++;
                data[dataIndex] |= (int16_t)rxMsg.data[currentIndex + i] << 8;
                i++;
            }
            
            break;
        }
        
        case eU32:
        {
            // typecast to a int
            uint32_t* data = (uint32_t*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                uint16_t dataIndex = i >> 2;
                data[dataIndex] = (uint32_t)rxMsg.data[currentIndex + i];
                i++;
                data[dataIndex] |= (uint32_t)rxMsg.data[currentIndex + i] << 8;
                i++;
                data[dataIndex] |= (uint32_t)rxMsg.data[currentIndex + i] << 16;
                i++;
                data[dataIndex] |= (uint32_t)rxMsg.data[currentIndex + i] << 24;
                i++;
            }
            
            break;
        }
        
        case eS32:
        {
            // typecast to a int
            int32_t* data = (int32_t*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                uint16_t dataIndex = i >> 2;
                data[dataIndex] = (int32_t)rxMsg.data[currentIndex + i];
                i++;
                data[dataIndex] |= (int32_t)rxMsg.data[currentIndex + i] << 8;
                i++;
                data[dataIndex] |= (int32_t)rxMsg.data[currentIndex + i] << 16;
                i++;
                data[dataIndex] |= (int32_t)rxMsg.data[currentIndex + i] << 24;
                i++;
            }
            break;
        }
        
        case eFLOAT:
        {
            
            break;
        }
    }
    
    return rxMsg.length;
}

uint16_t getCurrentRxPointerIndex(uint8_t element){
    uint16_t i = 0;
    uint16_t currentIndex = 0;
    
    for(i = 0; i < element; i++){
        uint16_t widthInBytes = 0;
        if((rxMsg.formatSpecifiers[i] == eNONE)
                || (rxMsg.formatSpecifiers[i] == eU8)
                || (rxMsg.formatSpecifiers[i] == eS8)){
            widthInBytes = 1;
        }else if((rxMsg.formatSpecifiers[i] == eU16)
                || (rxMsg.formatSpecifiers[i] == eS16)){
            widthInBytes = 2;
        }else{
            widthInBytes = 4;
        }
        
        currentIndex += rxMsg.length * widthInBytes;
    }
    
    return currentIndex;
}