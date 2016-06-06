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

typedef enum vimode{OFFSET_CALIBRATION, TWO_TERMINAL, THREE_TERMINAL}ViMode;

/*********** Variable Declarations ********************************************/
volatile q16angle_t theta = 0, omega = HIGH_SPEED_THETA_INCREMENT;
volatile q15_t loadVoltageL = 0;
volatile int16_t loadVoltage[NUM_OF_SAMPLES] = {0};
volatile int16_t loadCurrent[NUM_OF_SAMPLES] = {0};
volatile q15_t gateVoltage = 0;
volatile q15_t sampleIndex = 0;
volatile q15_t dacSamplesPerAdcSamples = 1;
volatile q15_t currentOffset = 0;

volatile ViMode mode = TWO_TERMINAL;
volatile uint8_t xmitActive = 0, xmitSent = 0;

q15_t gateVoltageSetpoint = 0;

/*********** Function Declarations ********************************************/
void initOsc(void);
void initLowZAnalogOut(void);
void initInterrupts(void);
void initPwm(void);
void initAdc(void);
void setDutyCyclePWM1(q15_t dutyCycle);
void setDutyCyclePWM2(q15_t dutyCycle);
q15_t getDutyCyclePWM2(void);

void sendVI(void);
void sendPeriod(void);
void sendGateVoltage(void);
void sendMode(void);

void changePeriod(void);
void receiveOffsetCalibration(void);
void setGateVoltage(void);
void toggleMode(void);

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
    DIS_subscribe("gate voltage", &setGateVoltage);
    DIS_subscribe("mode", &toggleMode);
    
    /* add necessary tasks */    
    TASK_add(&DIS_process, 1);
    TASK_add(&sendVI, 500);
    TASK_add(&sendPeriod, 999);
    TASK_add(&sendGateVoltage, 499);
    TASK_add(&sendMode, 498);
    
    TASK_manage();
    
    return 0;
}
/******************************************************************************/
/* Tasks below this line */
void sendVI(void){
    uint16_t i;
    
    xmitActive = 1;
    
    if(mode == TWO_TERMINAL){
        /* apply the currentOffset to each sample */
        for(i=0; i < NUM_OF_SAMPLES; i++){
            loadCurrent[i] -= currentOffset;
        }
    }else if(mode == OFFSET_CALIBRATION){
        /* find the average of the total number of samples */
        int32_t total = 0;
        uint16_t shift = 0;
        
        /* find the number of shifts that will be necessary */
        uint16_t numOfSamples = NUM_OF_SAMPLES;
        while(numOfSamples > 1){
            numOfSamples >>= 1;
            shift += 1;
        }
        
        /* find the total of all of the samples */
        for(i=0; i < NUM_OF_SAMPLES; i++){
            total += (int32_t)(loadCurrent[i]);
        }
        
        /* divide by shifting */
        while(shift > 0){
            total >>= 1;
            shift -= 1;
        }
        
        currentOffset = (q15_t)total;
        
        mode = TWO_TERMINAL;
    }
    
    DIS_publish_2s16("vi:64", (int16_t*)loadVoltage, (int16_t*)loadCurrent);
    xmitSent = 1;
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

void sendGateVoltage(void){
    DIS_publish_s16("gate voltage", (int16_t*)&gateVoltage);
    
    /* when the gate voltage is sent, then add or subtract a small amount to the
     * PWM based on the error */
    q15_t error = q15_add(gateVoltage, -gateVoltageSetpoint);

    q15_t dc = q15_add(getDutyCyclePWM2(), -error);

    setDutyCyclePWM2(dc);
}

void sendMode(void){
    if(mode == TWO_TERMINAL)
        DIS_publish_str("mode", "2");
    else if(mode == THREE_TERMINAL)
        DIS_publish_str("mode", "3");
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
    mode = OFFSET_CALIBRATION;
}

void setGateVoltage(void){
    DIS_getElements(0, &gateVoltageSetpoint);
    
    setDutyCyclePWM2(gateVoltageSetpoint);
}

void toggleMode(void){
    if(mode == TWO_TERMINAL){
        mode = THREE_TERMINAL;
    }else{
        mode = TWO_TERMINAL;
    }
}

/******************************************************************************/
/* Helper functions below this line */
void setDutyCyclePWM1(q15_t dutyCycle){
    CCP1RB = q15_mul(dutyCycle, CCP1PRL);
    
    /* when CCPxRB < 2, PWM doesn't update properly */
    if(CCP1RB < 2)
        CCP1RB = 2;
    else if(CCP1RB >= CCP1PRL)
        CCP1RB = CCP1PRL - 1;
    
    return;
}

void setDutyCyclePWM2(q15_t dutyCycle){
    CCP2RB = q15_mul(dutyCycle, CCP2PRL);
    
    /* when CCPxRB == 0, PWM doesn't update properly */
    if(CCP2RB < 2)
        CCP2RB = 2;
    else if(CCP2RB >= CCP2PRL)
        CCP2RB = CCP2PRL - 1;
    
    return;
}

q15_t getDutyCyclePWM2(void){
    return q15_div((q15_t)CCP2RB, (q15_t)CCP2PRL);
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
    
    /* initialize the period */
    dacSamplesPerAdcSamples = 1;
    omega = HIGH_SPEED_THETA_INCREMENT;
    theta = 0;
    PR1 = 1567;
    
    /* timer interrupts */
    T1CON = 0x0000;
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
    DIO_makeInput(DIO_PORT_A, 4);
    DIO_makeInput(DIO_PORT_B, 8);
    
    DIO_makeAnalog(DIO_PORT_A, 1);
    DIO_makeAnalog(DIO_PORT_B, 0);
    DIO_makeAnalog(DIO_PORT_A, 4);
    DIO_makeAnalog(DIO_PORT_B, 8);
    
    AD1CON1 = 0x0200;   /* Clear sample bit to trigger conversion
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
    static int countUp = 0;
    theta += omega;
    
    if(mode == THREE_TERMINAL){
        /* in 'transistor' mode, the gate and source voltages are held constant
         * while the drain voltage is adjusted */
        DAC1DAT = 0;
        
        if(countUp){
            uint32_t dac = theta;
            if(dac == 0)
                dac = 65535;
            DAC2DAT = dac;
        }else{
            uint32_t dac = (uint32_t)65536 - (uint32_t)theta;
            DAC2DAT = (uint16_t)(dac);
        }
    }else{
        DAC1DAT = q15_fast_sin(theta) + 32768;
        DAC2DAT = q15_fast_sin(theta + 32768) + 32768; // theta + 180 deg
    }
    
    /* reset sampleIndex on every cycle */
    if(theta == 0){
        if((xmitActive == 0) && (xmitSent == 1)){
            sampleIndex = 0;
            xmitSent = 0;
            LATBbits.LATB9 = 1;
        }
        
        AD1CON1bits.SAMP = 0;
        countUp ^= 1;
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
                int16_t sample = ((ADC1BUF0 >> 1) - loadVoltageL);
                loadVoltage[sampleIndex] = sample;
            }
            
            AD1CHS = CURRENT_VOLTAGE_AN;
            AD1CON1bits.SAMP = 0;

            break;
        }

        case CURRENT_VOLTAGE_AN:
        {
            if((sampleIndex < NUM_OF_SAMPLES) && (xmitActive == 0)){
                loadCurrent[sampleIndex] = (int16_t)((ADC1BUF0 >> 1) - 16384);
            }

            sampleIndex++;
            if(sampleIndex >= NUM_OF_SAMPLES){
                sampleIndex = NUM_OF_SAMPLES;
                LATBbits.LATB9 = 0;
            }

            AD1CHS = GATE_VOLTAGE_AN;
            AD1CON1bits.SAMP = 0;

            break;
        }

        case GATE_VOLTAGE_AN:
        {
            gateVoltage = (q15_t)(ADC1BUF0 >> 1);
            AD1CHS = LD_VOLTAGE_1_AN;

            break;
        }

        default: {}
    }
    
    /* clear the flag */
    IFS0bits.AD1IF = 0;
}

