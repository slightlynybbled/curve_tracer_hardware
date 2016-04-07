#ifndef _PUB_SERIAL_H
#define _PUB_SERIAL_H

#include <stdint.h>

/** The max number of dimensions that will be utilized */
#define MAX_NUM_OF_FORMAT_SPECIFIERS    4

/** The maximum number of subscriptions that will be utilized */
#define MAX_NUM_OF_SUBSCRIPTIONS        4

/** The maximum topic string length */
#define MAX_TOPIC_STR_LEN               16

/** The maximum transmission message length */
#define MAX_TRANSMIT_MESSAGE_LEN        64

/** The maximum receive message length */
#define MAX_RECEIVE_MESSAGE_LEN         32

/**
 * Initializes the PUB library elements, must be called before
 * any other PUB functions
 */
void PUB_init(void);

/**
 * Publish data to a particular topic
 * 
 * @param topic a text string that contains the topic, length,
 * and format specifiers
 * 
 * @param ... one or more pointers to the data arrays
 */
void PUB_publish(const char* topic, ...);

/**
 * Subscribe to a particular topic
 * 
 * @param topic a text string that contains the topic only
 * 
 * @param functPtr a function pointer to the function that should
 * be executed when the particular topic is received.
 */
void PUB_subscribe(const char* topic, void (*functPtr)());

/**
 * Unsubscribe the function from all topics
 * 
 * @param functPtr a function pointer to the function that should
 * be removed from all topics
 */
void PUB_unsubscribe(void (*functPtr)());

/**
 * This function must be called periodically in order to properly
 * call the subscribed function callbacks.  The minimum rate that 
 * the function should be called is dependant on expected topic
 * reception frequency.  This function should be called at a rate
 * at which a particular topic cannot be received twice.  For
 * instance, if your max-rate topic is sent every 100ms, then this
 * function should be called - at minimum - every 100ms.  You should
 * strive for twice this frequency, or 50ms, where possible.
 */
void PUB_process(void);

/**
 * The subscribing function(s) will use this function in order to
 * extract the received data from the publish library.  Note that
 * the subscribing functions must already be aware of the data type
 * that it is expecting.
 * 
 * @param element the element number
 * @param destArray the destination array for the data to be copied
 * into
 * @return the length of the received data
 */
uint16_t PUB_getElements(uint16_t element, void* destArray);

#endif