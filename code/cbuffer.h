#ifndef _CIRCULAR_BUFFER_H
#define _CIRCULAR_BUFFER_H

#include <stdint.h>

typedef enum bufferstatus {
    BUFFER_OK, 
    BUFFER_EMPTY, 
    BUFFER_FULL
} BufferStatus;

typedef struct buffer {
	uint16_t length;
    uint8_t width;
	uint16_t newest_index;
	uint16_t oldest_index;
    BufferStatus status;
    void* dataPtr;
} Buffer;

BufferStatus BUF_init(Buffer* b, void* arr, uint16_t length, uint8_t width);
BufferStatus BUF_status(Buffer* b);
int32_t BUF_emptySlots(Buffer* b);
int32_t BUF_fullSlots(Buffer* b);

BufferStatus BUF_write8(Buffer* b, uint8_t writeValue);
BufferStatus BUF_write16(Buffer* b, uint16_t writeValue);
BufferStatus BUF_write32(Buffer* b, uint32_t writeValue);

uint8_t BUF_read8(Buffer* b);
uint16_t BUF_read16(Buffer* b);
uint32_t BUF_read32(Buffer* b);



#endif
