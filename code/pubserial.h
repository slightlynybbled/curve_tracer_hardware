#ifndef _PUB_SERIAL_H
#define _PUB_SERIAL_H

void pubserial_init(void);
void publish(const char* topic, ...);
void subscribe(const char* topic, void (*functPtr)());
void unsubscribe(void (*functPtr)());
void process(void);

#endif