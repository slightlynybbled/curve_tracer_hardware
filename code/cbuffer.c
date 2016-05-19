#include "cbuffer.h"

BufferStatus BUF_init(Buffer* b, void* arr, uint16_t length, uint8_t width){
    /* ensure that the length is a power of 2 */
    uint8_t lengthOk = 0;
    uint16_t possibleLength = 1;
    uint16_t i;
    while(possibleLength < 16384){
        if(possibleLength == length){
            lengthOk = 1;
        }
        possibleLength = possibleLength << 1;
    }
    
    if(lengthOk == 0)
        while(1);   /* programmer's trap */
        
    /* ensure that the width is 8, 16, or 32 bits */
    if(width == 8){
        uint8_t* dataBuf = (uint8_t*)arr;
        b->dataPtr = dataBuf;
        
        /* erase the buffer */
        for(i = 0; i < length; i++){
            dataBuf[i] = 0;
        }
    }else if(width == 16){
        uint16_t* dataBuf = (uint16_t*)arr;
        b->dataPtr = dataBuf;
    }else if(width == 32){
        uint32_t* dataBuf = (uint32_t*)arr;
        b->dataPtr = dataBuf;
    }else{
        while(1);   /* programmer's trap */
    }
    
    b->dataPtr = arr;
    b->length = length;
    b->width = width;
    b->newest_index = 0;
    b->oldest_index = 0;
    b->status = BUFFER_EMPTY;
    
    return b->status;
}

BufferStatus BUF_status(Buffer* b){
    return b->status;
}

int32_t BUF_emptySlots(Buffer* b){
    int16_t emptySlots = 0;
    
    if(b->status == BUFFER_EMPTY){
        emptySlots = b->length;
    }else if(b->status == BUFFER_FULL){
        emptySlots = 0;
    }else{
        /* count the empty slots */
        if(b->newest_index > b->oldest_index){
            emptySlots = (b->length - b->newest_index)
                        + b->oldest_index;
        }else{
            emptySlots = b->oldest_index - b->newest_index;
        }
    }
    
    return emptySlots;
}

int32_t BUF_fullSlots(Buffer* b){
    int16_t fullSlots = 0;
    
    if(b->status == BUFFER_EMPTY){
        fullSlots = 0;
    }else if(b->status == BUFFER_FULL){
        fullSlots = b->length;
    }else{
        /* count the full slots */
        if(b->oldest_index > b->newest_index){
            fullSlots = (b->length - b->oldest_index)
                        + b->newest_index;
        }else{
            fullSlots = b->newest_index - b->oldest_index;
        }
    }
    
    return fullSlots;
}

BufferStatus BUF_write8(Buffer* b, uint8_t writeValue){
    if(b->status != BUFFER_FULL){
        uint8_t* dataBuf = (uint8_t*)b->dataPtr;
        dataBuf[b->newest_index] = writeValue;
        b->newest_index += 1;
        b->newest_index &= ~b->length;
        
        if(b->newest_index == b->oldest_index)
            b->status = BUFFER_FULL;
        else
            b->status = BUFFER_OK;
    }
    
    return b->status;
}

BufferStatus BUF_write16(Buffer* b, uint16_t writeValue){
    if(b->status != BUFFER_FULL){
        uint16_t* dataBuf = (uint16_t*)b->dataPtr;
        dataBuf[b->newest_index] = writeValue;
        b->newest_index += 1;
        b->newest_index &= ~b->length;
        
        if(b->newest_index == b->oldest_index)
            b->status = BUFFER_FULL;
        else
            b->status = BUFFER_OK;
    }
    
    return b->status;
}

BufferStatus BUF_write32(Buffer* b, uint32_t writeValue){
    if(b->status != BUFFER_FULL){
        uint32_t* dataBuf = (uint32_t*)b->dataPtr;
        dataBuf[b->newest_index] = writeValue;
        b->newest_index += 1;
        b->newest_index &= ~b->length;
        
        if(b->newest_index == b->oldest_index)
            b->status = BUFFER_FULL;
        else
            b->status = BUFFER_OK;
    }
    
    return b->status;
}

uint8_t BUF_read8(Buffer* b){
    uint8_t readValue = 0;
    
    if(b->status != BUFFER_EMPTY){
        uint8_t* dataBuf = (uint8_t*)b->dataPtr;
        readValue = dataBuf[b->oldest_index];
        b->oldest_index++;
        b->oldest_index &= ~b->length;
        
        if(b->newest_index == b->oldest_index)
            b->status = BUFFER_EMPTY;
        else
            b->status = BUFFER_OK;
    }
        
    return readValue;
}

uint16_t BUF_read16(Buffer* b){
    uint8_t readValue = 0;
    
    if(b->status != BUFFER_EMPTY){
        uint16_t* dataBuf = (uint16_t*)b->dataPtr;
        readValue = dataBuf[b->oldest_index];
        b->oldest_index++;
        b->oldest_index &= ~b->length;
        
        if(b->newest_index == b->oldest_index)
            b->status = BUFFER_EMPTY;
        else
            b->status = BUFFER_OK;
    }
        
    return readValue;
}

uint32_t BUF_read32(Buffer* b){
    uint8_t readValue = 0;
    
    if(b->status != BUFFER_EMPTY){
        uint32_t* dataBuf = (uint32_t*)b->dataPtr;
        readValue = dataBuf[b->oldest_index];
        b->oldest_index++;
        b->oldest_index &= ~b->length;
        
        if(b->newest_index == b->oldest_index)
            b->status = BUFFER_EMPTY;
        else
            b->status = BUFFER_OK;
    }
        
    return readValue;
}





