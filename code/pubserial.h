#ifndef _PUB_SERIAL_H
#define _PUB_SERIAL_H

#include <stdint.h>

void pubserial_init(void);
void publish(const char* topic, ...);
void subscribe(const char* topic, void (*functPtr)());
void unsubscribe(void (*functPtr)());
void process(void);
uint16_t SUB_getElements(uint16_t element, void* destArray);

#endif