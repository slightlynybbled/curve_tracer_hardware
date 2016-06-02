/*
 * File:   main.c
 * Author: Jason
 *
 * Created on February 13, 2016, 10:43 AM
 */


#include "config.h"
#include "libmathq15.h"
#include "task.h"
#include "dispatch.h"
#include "dio.h"
#include "uart.h"
#include <string.h>

/*********** Useful defines and macros ****************************************/
#define LD_VOLTAGE_1_AN     0x0101
#define LD_VOLTAGE_0_AN     0x0202
#define GATE_VOLTAGE_AN     0x1010
#define CURRENT_VOLTAGE_AN  0x1414

#define NUM_OF_SAMPLES                  64
#define HIGH_SPEED_THETA_INCREMENT     (65536/NUM_OF_SAMPLES)

typedef enum vimode{OFFSET_CALIBRATION, PROBE, TRANSISTOR}ViMode;

/*********** Variable Declarations ********************************************/
volatile q16angle_t theta = 0, omega = HIGH_SPEED_THETA_INCREMENT;
volatile q15_t loadVoltageL = 0;
volatile int8_t loadVoltage[NUM_OF_SAMPLES] = {0};
volatile int8_t loadCurrent[NUM_OF_SAMPLES] = {0};
volatile q15_t gateVoltage = 0;
volatile q15_t sampleIndex = 0;
volatile q15_t dacSamplesPerAdcSamples = 1;

volatile ViMode mode = PROBE;
volatile uint8_t xmitActive = 0;

/*********** Function Declarations ********************************************/
void initOsc(void);
void initLowZAnalogOut(void);
void initInterrupts(void);
void initPwm(void);
void initAdc(void);
void setDutyCyclePWM1(q15_t dutyCycle);
void setDutyCyclePWM2(q15_t dutyCycle);

void sendVI(void);
void sendPeriod(void);
void changePeriod(void);
void receiveOffsetCalibration(void);

/*********** Function Implementations *****************************************/
int main(void) {
    /* setup the hardware */
    initOsc();
    initLowZAnalogOut();
    initInterrupts();
    initPwm();
    initAdc();
    
    DIO_makeOutput(DIO_PORT_B, 9);  /* use for debugging */
    
    UART_init();
    DIS_assignChannelReadable(&UART_readable);
    DIS_assignChannelWriteable(&UART_writeable);
    DIS_assignChannelRead(&UART_read);
    DIS_assignChannelWrite(&UART_write);
    DIS_init();
    
    /* initialize the task manager */
    TASK_init();
    
    /* set the initial duty cycles */
    setDutyCyclePWM1(16384);
    setDutyCyclePWM2(8192);
    
    /* add Dispatch subscribers */
    DIS_subscribe("period", &changePeriod);
    DIS_subscribe("cal", &receiveOffsetCalibration);

    /* add necessary tasks */    
    TASK_add(&DIS_process, 1);
    TASK_add(&sendVI, 500);
    TASK_add(&sendPeriod, 1000);
    
    TASK_manage();
    
    return 0;
}
/******************************************************************************/
/* Tasks below this line */
void sendVI(void){
    xmitActive = 1;
    DIS_publish_2s8("vi:64", (int8_t*)loadVoltage, (int8_t*)loadCurrent);
    xmitActive = 0;
}

void sendPeriod(void){
    uint16_t period = PR1;
    
    int i = 1;
    do{
        period <<= 1;
        i++;
    }while(i < dacSamplesPerAdcSamples);
    
    DIS_publish_u16("period", &period);
}

/******************************************************************************/
/* Subscribers below this line */
void changePeriod(void){
    uint16_t newPeriod;
    q16angle_t newOmega = HIGH_SPEED_THETA_INCREMENT;
    dacSamplesPerAdcSamples = 1;
    
    DIS_getElements(0, &newPeriod);
    
    while(newPeriod > 2000){
        newPeriod >>= 1;
        newOmega >>= 1;
        dacSamplesPerAdcSamples++;
    }
    
    omega = newOmega;
    theta = 0;
    PR1 = newPeriod;
}

void receiveOffsetCalibration(void){

}

/******************************************************************************/
/* Helper functions below this line */
void setDutyCyclePWM1(q15_t dutyCycle){
    CCP1RB = q15_mul(dutyCycle, CCP1PRL);
    return;
}

void setDutyCyclePWM2(q15_t dutyCycle){
    CCP2RB = q15_mul(dutyCycle, CCP2PRL);
    return;
}

/******************************************************************************/
/* Initialization functions below this line */
void initOsc(void){
    /* for the moment, initialize the oscillator
     * on the highest internal frequency;  this will likely
     * change soon */
    CLKDIV = 0;

    return;
}

void initLowZAnalogOut(void){
    /* both DACs must be initialized, connected to external pins */
    DIO_makeInput(DIO_PORT_B, 3);
    DIO_makeAnalog(DIO_PORT_B, 3);
    DIO_makeInput(DIO_PORT_B, 15);
    DIO_makeAnalog(DIO_PORT_B, 15);
    
    /* DAC config:
     * trigger on write, DAC available to internal
     * peripherals only, sleep behavior doesn't matter,
     * left-aligned input (fractional) */
    DAC1CON = DAC2CON = 0x0802;
    DAC1CONbits.DACEN = DAC2CONbits.DACEN = 1; // enable after configured
    
    /* Opamp config:
     * higher bandwidth/response, voltage follower config,
     * positive input connected to DAC */
    AMP1CON = AMP2CON = 0x002D;
    AMP1CONbits.AMPEN = AMP2CONbits.AMPEN = 1;
    
    return;
}

void initInterrupts(void){
    /* configure the global interrupt conditions */
    /* interrupt nesting disabled, DISI instruction active */
    INTCON1 = 0x8000;
    INTCON2 = 0x4000;
    
    /* timer interrupts */
    T1CON = 0x0000;
    PR1 = 1000;
    IFS0bits.T1IF = 0;
    IEC0bits.T1IE = 1;
    T1CONbits.TON = 1;
    
    return;
}

void initPwm(void){
    // use OC1C (pin 21, RB10) and OC2A (pin 22, RB11)
    DIO_makeDigital(DIO_PORT_B, 10);
    DIO_makeDigital(DIO_PORT_B, 11);
    
    /* Initialize MCCP module
     *  */
    /* period registers */
    CCP1PRH = CCP2PRH = 0;
    CCP1PRL = CCP2PRL = 1024;
    
    CCP1CON1L = CCP2CON1L = 0x0005;
    CCP1CON1H = CCP2CON1H = 0x0000;
    CCP1CON2L = CCP2CON2L = 0x0000;
    CCP1CON2H = 0x8400; // enable output OC1C
    CCP2CON2H = 0x8100; // enable output 0C2A
    CCP1CON3L = CCP2CON3L = 0;  // dead time disabled
    CCP1CON3H = CCP2CON3H = 0x0000;
    
    CCP1CON1Lbits.CCPON = CCP2CON1Lbits.CCPON = 1;
    
    /* duty cycle registers */
    CCP1RA = CCP2RA = 0;
    CCP1RB = CCP2RB = 0;
    
    return;
}

void initAdc(void){
    /* set up the analog pins as analog inputs */
    DIO_makeInput(DIO_PORT_A, 1);
    DIO_makeInput(DIO_PORT_B, 0);
    DIO_makeInput(DIO_PORT_B, 4);
    DIO_makeInput(DIO_PORT_A, 4);
    DIO_makeInput(DIO_PORT_B, 6);
    
    DIO_makeAnalog(DIO_PORT_A, 1);
    DIO_makeAnalog(DIO_PORT_B, 0);
    DIO_makeAnalog(DIO_PORT_B, 4);
    DIO_makeAnalog(DIO_PORT_A, 4);
    DIO_makeAnalog(DIO_PORT_B, 6);
    
    AD1CON1 = 0x0200;   /* Internal counter triggers conversion
                         * FORM = left justified  */
    AD1CON2 = 0x0000;   /* Set AD1IF after every 1 samples */
    AD1CON3 = 0x0007;   /* Sample time = 1Tad, Tad = 8 * Tcy */
    
    AD1CHS = CURRENT_VOLTAGE_AN;    /* AN1 */
    AD1CSSL = 0;
    
    AD1CON1bits.ADON = 1; // turn ADC ON
    AD1CON1bits.ASAM = 1; // auto-sample
    
    /* analog-to-digital interrupts */
    IFS0bits.AD1IF = 0;
    IEC0bits.AD1IE = 1;
    
    return;
}

/******************************************************************************/
/* Interrupt functions below this line */
/**
 * The T1Interrupt will be used to load the DACs and generate the sine wave
 */
void _ISR _T1Interrupt(void){
    theta += omega;
    
    DAC1DAT = q15_fast_sin(theta) + 32768;
    DAC2DAT = q15_fast_sin(theta + 32768) + 32768; // theta + 180 deg
    
    /* reset sampleIndex on every cycle */
    if(theta == 0){
        if(xmitActive == 0){
            sampleIndex = 0;
        }
        
        AD1CON1bits.SAMP = 0;
    }else if((theta & (HIGH_SPEED_THETA_INCREMENT-1)) == 0){
        AD1CON1bits.SAMP = 0;
    }
    
    IFS0bits.T1IF = 0;
    
    return;
}

void _ISR _ADC1Interrupt(void){
    switch(AD1CHS){
        case LD_VOLTAGE_1_AN:
        {
            loadVoltageL = (q15_t)(ADC1BUF0 >> 1);
            AD1CHS = LD_VOLTAGE_0_AN;
            AD1CON1bits.SAMP = 0;

            break;
        }

        case LD_VOLTAGE_0_AN:
        {
            if((sampleIndex < NUM_OF_SAMPLES) && (xmitActive == 0)){
                int8_t sample = (int8_t)(((ADC1BUF0 >> 1) - loadVoltageL) >> 8);
                loadVoltage[sampleIndex] = sample;
            }
            
            AD1CHS = CURRENT_VOLTAGE_AN;
            AD1CON1bits.SAMP = 0;

            break;
        }

        case CURRENT_VOLTAGE_AN:
        {
            if((sampleIndex < NUM_OF_SAMPLES) && (xmitActive == 0)){
                LATBbits.LATB9 = 1;
                loadCurrent[sampleIndex] = (int8_t)(((ADC1BUF0 >> 1) - 16384) >> 8);
            }

            sampleIndex++;
            if(sampleIndex >= NUM_OF_SAMPLES){
                sampleIndex = NUM_OF_SAMPLES;
            }

            AD1CHS = GATE_VOLTAGE_AN;
            AD1CON1bits.SAMP = 0;

            break;
        }

        case GATE_VOLTAGE_AN:
        {
            gateVoltage = (q15_t)(ADC1BUF0 >> 1);
            AD1CHS = LD_VOLTAGE_1_AN;
            
            LATBbits.LATB9 = 0;

            break;
        }

        default: {}
    }
    
    /* clear the flag */
    IFS0bits.AD1IF = 0;
}

