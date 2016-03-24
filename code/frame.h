#ifndef _FRAME_H
#define _FRAME_H

#include <stdint.h>
#include <stdbool.h>

void FRM_init(void);
void FRM_push(uint8_t* data, uint16_t length);
uint16_t FRM_pull(uint8_t* data);

#endif