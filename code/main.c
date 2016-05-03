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
#define LD_VOLTAGE_1_AN 0x0101
#define LD_VOLTAGE_0_AN 0x0202
#define HZ_VOLTAGE_1_AN 0x0f0f
#define HZ_VOLTAGE_2_AN 0x1010
#define CURRENT_VOLTAGE_AN  0x1414

#define NUM_OF_SAMPLES      64
#define THETA_SAMPLE_PERIOD (65536/NUM_OF_SAMPLES)

/*********** Variable Declarations ********************************************/
volatile q16angle_t omega = 60;
volatile q16angle_t theta = 0;

volatile q15_t loadVoltageL = 0;
volatile int8_t loadVoltage[NUM_OF_SAMPLES] = {0};
volatile int8_t loadCurrent[NUM_OF_SAMPLES] = {0};
volatile q15_t sampleIndex = 0;
volatile uint8_t sampleEnable = 0;
volatile uint8_t resetSampleIndexFlag = 0;
volatile q15_t hz1Voltage = 0;
volatile q15_t hz2Voltage = 0;

/*********** Function Declarations ********************************************/
void initOsc(void);
void initLowZAnalogOut(void);
void initInterrupts(void);
void initPwm(void);
void initAdc(void);
void setDutyCycleHZ1(q15_t dutyCycle);
void setDutyCycleHZ2(q15_t dutyCycle);

void sendVI(void);
void changeOmega(void);
void itoa10(int16_t value, char* str);

/*********** Function Implementations *****************************************/
int main(void) {
    /* setup the hardware */
    initOsc();
    initLowZAnalogOut();
    initInterrupts();
    initPwm();
    initAdc();
    
    UART_init();
    DIS_assignChannelReadable(&UART_readable);
    DIS_assignChannelWriteable(&UART_writeable);
    DIS_assignChannelRead(&UART_read);
    DIS_assignChannelWrite(&UART_write);
    DIS_init();
    
    /* initialize the task manager */
    TASK_init();
    
    /* set the initial duty cycles */
    setDutyCycleHZ1(16384);
    setDutyCycleHZ2(8192);
    
    /* add necessary tasks */
    DIS_subscribe("omega", &changeOmega);
    TASK_add(&DIS_process, 1);
    TASK_add(&sendVI, 500);
    
    TASK_manage();
    
    return 0;
}

void sendVI(void){
    resetSampleIndexFlag = 1;
    
    DIS_publish("vi:64,s8,s8", loadVoltage, loadCurrent);
}

void changeOmega(void){
    uint16_t newOmega;
    DIS_getElements(0, &newOmega);
    omega = newOmega;
}

void initOsc(void){
    /* for the moment, initialize the oscillator
     * on the highest internal frequency;  this will likely
     * change soon */
    CLKDIV = 0;

    return;
}

void initLowZAnalogOut(void){
    /* both DACs must be initialized, but do not
     * need to be connected to external pins */
    
    /* Pin config:
     * according to datasheet, opamp pins
     * should be in 'analog' mode as 'inputs' */
    DIO_makeInput(DIO_PORT_B, 3);
    DIO_makeInput(DIO_PORT_B, 15);
    DIO_makeInput(DIO_PORT_B, 14);
    DIO_makeAnalog(DIO_PORT_B, 3);
    DIO_makeAnalog(DIO_PORT_B, 15);
    
    
    /* DAC config:
     * trigger on write, DAC available to internal
     * peripherals only, sleep behavior doesn't matter,
     * left-aligned input (fractional) */
    DAC1CON = DAC2CON = 0x0802;
    //DAC1CONbits.DACOE = DAC2CONbits.DACOE = 1;
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
    PR1 = 1000;          // based on 12MIPS, 48samples/waveform, 1kHz waveform
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

void setDutyCycleHZ1(q15_t dutyCycle){
    CCP1RB = q15_mul(dutyCycle, CCP1PRL);
    return;
}

void setDutyCycleHZ2(q15_t dutyCycle){
    CCP2RB = q15_mul(dutyCycle, CCP2PRL);
    return;
}

/**
 * The T1Interrupt will be used to load the DACs and generate the sine wave
 */
void _ISR _T1Interrupt(void){
    static q16angle_t thetaLast = 0, thetaLastSample;
    thetaLast = theta;
    theta += omega;
    
    if((theta - thetaLastSample) > THETA_SAMPLE_PERIOD){
        sampleEnable = 1;
        thetaLastSample += THETA_SAMPLE_PERIOD;
    }
    
    DAC1DAT = q15_fast_sin(theta) + 32768;
    DAC2DAT = q15_fast_sin(theta + 32768) + 32768; // theta + 180 deg
    
    IFS0bits.T1IF = 0;
    AD1CON1bits.SAMP = 0;
    
    /* reset sampleIndex on every cycle */
    if((thetaLast > 32768) && (theta < 32768)){
        if(resetSampleIndexFlag == 1){
            sampleIndex = 0;
            resetSampleIndexFlag = 0;
        }
    }
    
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
            if(sampleEnable){
                if(sampleIndex < NUM_OF_SAMPLES)
                    loadVoltage[sampleIndex] = (int8_t)(((ADC1BUF0 >> 1) - loadVoltageL) >> 8);
            }
            
            AD1CHS = CURRENT_VOLTAGE_AN;
            AD1CON1bits.SAMP = 0;

            break;
        }

        case CURRENT_VOLTAGE_AN:
        {
            if(sampleEnable){
                if(sampleIndex < NUM_OF_SAMPLES)
                    loadCurrent[sampleIndex] = (int8_t)(((ADC1BUF0 >> 1) - hz1Voltage) >> 8);

                sampleIndex++;
                if(sampleIndex >= NUM_OF_SAMPLES){
                    sampleIndex = NUM_OF_SAMPLES;
                }
                
                sampleEnable = 0;
            }

            AD1CHS = HZ_VOLTAGE_1_AN;
            AD1CON1bits.SAMP = 0;
            

            break;
        }

        case HZ_VOLTAGE_1_AN:
        {
            hz1Voltage = (q15_t)(ADC1BUF0 >> 1);
            AD1CHS = HZ_VOLTAGE_2_AN;
            AD1CON1bits.SAMP = 0;

            break;
        }

        case HZ_VOLTAGE_2_AN:
        {
            hz2Voltage = (q15_t)(ADC1BUF0 >> 1);
            AD1CHS = LD_VOLTAGE_1_AN;

            break;
        }

        default: {}
    }
    
    /* clear the flag */
    IFS0bits.AD1IF = 0;
}

void itoa10(int16_t value, char* str){
    const uint8_t digitLookup[] = {'0', '1', '2', '3', '4',
                                    '5', '6', '7', '8', '9'};
    uint16_t i = 0;
    uint8_t digitValue = 0;
    
    /* for now, base == 10 */
    if(value & 0x8000){
        /* value is negative */
        str[i++] = '-';
        
        while(value < -10000){
            value += 10000;
            digitValue += 1;
        }
        str[i++] = digitLookup[digitValue];
        digitValue = 0;
        
        while(value < -1000){
            value += 1000;
            digitValue += 1;
        }
        str[i++] = digitLookup[digitValue];
        digitValue = 0;
        
        while(value < -100){
            value += 100;
            digitValue += 1;
        }
        str[i++] = digitLookup[digitValue];
        digitValue = 0;
        
        while(value < -10){
            value += 10;
            digitValue += 1;
        }
        str[i++] = digitLookup[digitValue];
        digitValue = 0;
        
        while(value < -1){
            value += 1;
            digitValue += 1;
        }
        str[i++] = digitLookup[digitValue];
    }else{
        /* value is positive */
        while(value > 10000){
            value -= 10000;
            digitValue += 1;
        }
        str[i++] = digitLookup[digitValue];
        digitValue = 0;
        
        while(value > 1000){
            value -= 1000;
            digitValue += 1;
        }
        str[i++] = digitLookup[digitValue];
        digitValue = 0;
        
        while(value > 100){
            value -= 100;
            digitValue += 1;
        }
        str[i++] = digitLookup[digitValue];
        digitValue = 0;
        
        while(value > 10){
            value -= 10;
            digitValue += 1;
        }
        str[i++] = digitLookup[digitValue];
        digitValue = 0;
        
        while(value > 1){
            value -= 1;
            digitValue += 1;
        }
        str[i++] = digitLookup[digitValue];
    }
    
    str[i] = 0; // terminate the string
}
