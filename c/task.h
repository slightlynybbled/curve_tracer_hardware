/*
 * task.h
 *
 *  Created on: Sep 27, 2015
 *      Author: Jason
 */

#ifndef TASK_H_
#define TASK_H_

#include <stdint.h>

void TASK_init();
void TASK_add(void (*functPtr)(), uint32_t period);
void TASK_remove(void (*functPtr)());
void TASK_manage();

uint32_t TASK_getTime();
void TASK_resetTime(uint32_t time);

#endif /* TASK_H_ */
