#ifndef _CIRCULAR_BUFFER_H
#define _CIRCULAR_BUFFER_H

#include <stdint.h>

/* The buffer length sets the number of elements
 * in the buffers.  This should always be a power
 * of 2 for this library */
#define BUFFER_LENGTH           128

/* valid values for BUFFER_ELEMENT_WIDTH are
 * 8, 16, and 32.  If you place another value
 * here, you will get a default of '8' */
#define BUFFER_ELEMENT_WIDTH    8

typedef enum bufferstatus {
    BUFFER_OK, 
    BUFFER_EMPTY, 
    BUFFER_FULL
} BufferStatus;

#if (BUFFER_ELEMENT_WIDTH == 32)
typedef struct buffer {
	uint32_t data[BUFFER_LENGTH];
	uint16_t newest_index;      // buffer could be long, use a 16-bit var here
	uint16_t oldest_index;
    BufferStatus status;
} Buffer;
#elif (BUFFER_ELEMENT_WIDTH == 16)
typedef struct buffer {
	uint16_t data[BUFFER_LENGTH];
	uint16_t newest_index;      // buffer could be long, use a 16-bit var here
	uint16_t oldest_index;
    BufferStatus status;
} Buffer;

#else
typedef struct buffer {
	uint8_t data[BUFFER_LENGTH];
	uint16_t newest_index;      // buffer could be long, use a 16-bit var here
	uint16_t oldest_index;
    BufferStatus status;
} Buffer;
#endif

BufferStatus BUF_init(Buffer* b);
BufferStatus BUF_status(Buffer* b);
int16_t BUF_emptySlots(Buffer* b);
int16_t BUF_fullSlots(Buffer* b);

#if (BUFFER_ELEMENT_WIDTH == 32)
BufferStatus BUF_write(Buffer* b, uint32_t writeValue);
uint32_t BUF_read(Buffer* b);

#elif (BUFFER_ELEMENT_WIDTH == 16)
BufferStatus BUF_write(Buffer* b, uint16_t writeValue);
uint16_t BUF_read(Buffer* b);

#else
BufferStatus BUF_write(Buffer* b, uint8_t writeValue);
uint8_t BUF_read(Buffer* b);

#endif

#endif
