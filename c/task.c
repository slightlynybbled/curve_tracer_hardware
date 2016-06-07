/*
 * task.c
 *
 *  Created on: Sep 27, 2015
 *      Author: Jason
 */

#include <xc.h>
#include "task.h"

#define MAX_NUM_OF_TASKS	10
#define MAX_SYS_TICKS_VAL	0x7ff00000

/* create structure that consists of a function pointer and period */
typedef struct {
	void (*taskFunctPtr)();
	uint32_t period;
	uint32_t nextExecutionTime;
}Task;

static Task task[MAX_NUM_OF_TASKS];
static volatile uint32_t systemTicks = 0;

void (*TMR_timedFunctPtr)();

void TASK_systemTicksCounter();	// function declaration
void TMR_init(void (*functPtr)());
void TMR_disableInterrupt();

void TASK_systemTicksCounter(){
	systemTicks++;

	/* gracefully reset the system ticks counter
	 * if it is about to roll over */
	if(systemTicks >= MAX_SYS_TICKS_VAL){
		TASK_resetTime(0);
	}
}

uint32_t TASK_getTime(){
    uint8_t sameFlag = 0;
    uint32_t now0 = systemTicks;
    uint32_t now1 = systemTicks;
    
    /* this code ensures that two sequential reads get the same value
     * before returning in order to avoid problems with atomic reads
     * of the systemTicks location (a 32-bit value on a 16-bit processor) */
    while(sameFlag == 0){
        now0 = systemTicks;
        now1 = systemTicks;
        
        if(now0 == now1){
            sameFlag = 1;
        }
    }
    
	return now0;
}

void TASK_resetTime(uint32_t time){
    uint32_t now = TASK_getTime();
	if(time != now){
		uint8_t i = 0;

		/* determine the amount of time before each task needs to execute again */
		int32_t timeUntilNextExecution[MAX_NUM_OF_TASKS];
		for(i = 0; i < MAX_NUM_OF_TASKS; i++){
			timeUntilNextExecution[i] = (int32_t)task[i].nextExecutionTime - now;
			if(timeUntilNextExecution[i] < 0)
				timeUntilNextExecution[i] = 0;
		}

        /* reset the clock */
		TMR_disableInterrupt();
		systemTicks = time;
        TMR_init(&TASK_systemTicksCounter);

		/* place the difference between the time that each task needed to execute and the new time */
		for(i = 0; i < MAX_NUM_OF_TASKS; i++){
			task[i].nextExecutionTime = (uint32_t)timeUntilNextExecution[i] + time;
		}
	}
}

void TASK_init(){
    TMR_init(&TASK_systemTicksCounter);

    /* initialize all of the tasks */
    uint16_t i;
    for(i = 0; i < MAX_NUM_OF_TASKS; i++){
    	task[i].taskFunctPtr = 0;
    	task[i].period = 1;
    	task[i].nextExecutionTime = 1;
    }
}

void TASK_add(void (*functPtr)(), uint32_t period){
	uint16_t i;
	uint8_t taskExists = 0;

	/* ensure that the task does not currently already exist with the same period */
	for(i = 0; i < MAX_NUM_OF_TASKS; i++){
		if(task[i].taskFunctPtr == functPtr){
			if(task[i].period != period){
				task[i].period = period;
				task[i].nextExecutionTime = systemTicks + period;
			}

			taskExists = 1;
		}
	}

	/* save the task */
	if(taskExists == 0){
		for(i = 0; i < MAX_NUM_OF_TASKS; i++){
			/* look for an empty task */
			if(task[i].taskFunctPtr == 0){
				task[i].taskFunctPtr = functPtr;
				task[i].period = period;
				task[i].nextExecutionTime = systemTicks + period;

				break;
			}
		}
	}
}

void TASK_remove(void (*functPtr)()){
	uint16_t i;

	for(i = 0; i < MAX_NUM_OF_TASKS; i++){
		if(task[i].taskFunctPtr == functPtr){
			task[i].taskFunctPtr = 0;
			task[i].period = 10000;
			task[i].nextExecutionTime = MAX_SYS_TICKS_VAL;
		}
	}
}

void TASK_manage(){
	while(1){
		uint16_t i;
		for(i = 0; i < MAX_NUM_OF_TASKS; i++){
			uint32_t time = TASK_getTime();
			if(task[i].taskFunctPtr != 0){
				if(time >= task[i].nextExecutionTime){
					task[i].nextExecutionTime = task[i].period + time;
					(task[i].taskFunctPtr)();
				}
			}
		}
        
        ClrWdt();
	}
}

void TMR_init(void (*functPtr)()){
	TMR_timedFunctPtr = functPtr;

    /* period registers */
    CCP3PRH = 0;
    CCP3PRL = 12000;
    
    CCP3CON1L = 0x0000; // timer mode
    CCP3CON1H = 0x0000;
    CCP3CON2L = 0x0000;
    CCP3CON2H = 0x0000;
    CCP3CON3L = 0;
    CCP3CON3H = 0x0000;
    
    IFS1bits.CCT3IF = 0;
    IEC1bits.CCT3IE = 1;
    
    CCP3CON1Lbits.CCPON = 1;
}

void TMR_disableInterrupt(){
    IEC1bits.CCT3IE = 0;
}

void _ISR _CCT3Interrupt(){
    if(TMR_timedFunctPtr != 0)
		(*TMR_timedFunctPtr)();
    
    IFS1bits.CCT3IF = 0;
}

