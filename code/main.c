/*
 * File:   main.c
 * Author: Jason
 *
 * Created on February 13, 2016, 10:43 AM
 */


#include <xc.h>
#include "libmathq15.h"

/********************* CONFIGURATION BIT SETTINGS *****************************/
// FBS
#pragma config BWRP = OFF               // Boot Segment Write Protect (Disabled)
#pragma config BSS = OFF                // Boot segment Protect (No boot program flash segment)

// FGS
#pragma config GWRP = OFF               // General Segment Write Protect (General segment may be written)
#pragma config GCP = OFF                // General Segment Code Protect (No Protection)

// FOSCSEL
#pragma config FNOSC = FRCPLL           // Oscillator Select (Fast RC Oscillator with Postscaler and PLL Module (FRCDIV+PLL))
#pragma config SOSCSRC = ANA            // SOSC Source Type (Analog Mode for use with crystal)
#pragma config LPRCSEL = LP             // LPRC Oscillator Power and Accuracy (Low Power, Low Accuracy Mode)
#pragma config IESO = ON                // Internal External Switch Over bit (Internal External Switchover mode enabled (Two-speed Start-up enabled))

// FOSC
#pragma config POSCMOD = NONE           // Primary Oscillator Configuration bits (Primary oscillator disabled)
#pragma config OSCIOFNC = CLKO          // CLKO Enable Configuration bit (CLKO output signal enabled)
#pragma config POSCFREQ = HS            // Primary Oscillator Frequency Range Configuration bits (Primary oscillator/external clock input frequency greater than 8MHz)
#pragma config SOSCSEL = SOSCHP         // SOSC Power Selection Configuration bits (Secondary Oscillator configured for high-power operation)
#pragma config FCKSM = CSECME           // Clock Switching and Monitor Selection (Both Clock Switching and Fail-safe Clock Monitor are enabled)

// FWDT
#pragma config WDTPS = PS32768          // Watchdog Timer Postscale Select bits (1:32768)
#pragma config FWPSA = PR128            // WDT Prescaler bit (WDT prescaler ratio of 1:128)
#pragma config FWDTEN = ON              // Watchdog Timer Enable bits (WDT enabled in hardware)
#pragma config WINDIS = OFF             // Windowed Watchdog Timer Disable bit (Standard WDT selected(windowed WDT disabled))

// FPOR
#pragma config BOREN = BOR3             // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware, SBOREN bit disabled)
#pragma config PWRTEN = ON              // Power-up Timer Enable bit (PWRT enabled)
#pragma config I2C1SEL = PRI            // Alternate I2C1 Pin Mapping bit (Use Default SCL1/SDA1 Pins For I2C1)
#pragma config BORV = V18               // Brown-out Reset Voltage bits (Brown-out Reset set to lowest voltage (1.8V))
#pragma config MCLRE = ON               // MCLR Pin Enable bit (RA5 input pin disabled, MCLR pin enabled)

// FICD
#pragma config ICS = PGx3               // ICD Pin Placement Select bits (EMUC/EMUD share PGC3/PGD3)

/****************************** END CONFIGURATION *****************************/

/*********** Useful defines and macros ****************************************/
#define DIO_OUTPUT  0
#define DIO_INPUT   1
#define DIO_DIGITAL 0
#define DIO_ANALOG  1

#define OMEGA_MIN_TO_FAST_SINE      2000
#define OMEGA_MAX_TO_NORMAL_SINE    (OMEGA_MIN_TO_FAST_SINE >> 1)

#define LD_VOLTAGE_1_AN 0x0101
#define LD_VOLTAGE_0_AN 0x0202
#define HZ_VOLTAGE_1_AN 0x0f0f
#define HZ_OVLTAGE_2_AN 0x1010
#define CURRENT_VOLTAGE_AN  0x1414

#define NUM_OF_SAMPLES 100

/*********** Variable Declarations ********************************************/
volatile q16angle_t omega = 2667;

volatile q15_t loadVoltageL = 0;
volatile q15_t loadVoltage[NUM_OF_SAMPLES] = {0};
volatile q15_t loadCurrent[NUM_OF_SAMPLES] = {0};
volatile q15_t sampleIndex = 0;
volatile q15_t hz1Voltage = 0;
volatile q15_t hz2Voltage = 0;

/*********** Function Declarations ********************************************/
void initOsc(void);
void initLowZAnalogOut(void);
void initInterrupts(void);
void initPwm(void);
void initAdc(void);
void initUart(void);
void setDutyCycleHZ1(q15_t dutyCycle);
void setDutyCycleHZ2(q15_t dutyCycle);


/*********** Function Implementations *****************************************/
int main(void) {
    initOsc();
    initLowZAnalogOut();
    initInterrupts();
    initPwm();
    initAdc();
    initUart();
    
    /* set the initial duty cycles */
    setDutyCycleHZ1(16384);
    setDutyCycleHZ2(8192);
    
    while(1){
        /* for the moment, the only background 
         * task is clearing the watchdog timer */
        ClrWdt();
    }
    
    return 0;
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
    TRISBbits.TRISB3 = TRISBbits.TRISB15 = TRISBbits.TRISB14 = DIO_INPUT;
    ANSBbits.ANSB3 = ANSBbits.ANSB15 = DIO_ANALOG;
    
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
    
    /* configure individual interrupts */
    
    /* timer interrupts */
    T1CON = 0x0000;
    PR1 = 250;          // based on 12MIPS, 48samples/waveform, 1kHz waveform
    IFS0bits.T1IF = 0;
    IEC0bits.T1IE = 1;
    T1CONbits.TON = 1;
    
    /* uart interrupts */
    IFS0bits.U1TXIF = IFS0bits.U1RXIF = 0;
    // IEC0bits.U1TXIE = IEC0bits.U1RXIE = 1;
    
    return;
}

void initPwm(void){
    // use OC1C (pin 21, RB10) and OC2A (pin 22, RB11)
    TRISBbits.TRISB10 = TRISBbits.TRISB11 = DIO_DIGITAL;
    
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
    TRISAbits.TRISA1 = 
            TRISBbits.TRISB0 = 
            TRISBbits.TRISB4 =
            TRISAbits.TRISA4 = 
            TRISBbits.TRISB6 = DIO_INPUT;
    ANSAbits.ANSA1 =
            ANSBbits.ANSB0 =
            ANSBbits.ANSB4 =
            ANSAbits.ANSA4 =
            ANSBbits.ANSB6 = DIO_ANALOG;
    
    AD1CON1 = 0x0270;   /* Internal counter triggers conversion
                         * FORM = left justified  */
    AD1CON2 = 0x003C;   /* Set AD1IF after every 16 samples */
    AD1CON3 = 0x0107;   /* Sample time = 1Tad, Tad = 8 * Tcy */
    
    AD1CHS = 0x0101;    /* AN1 */
    AD1CSSL = 0;
    
    AD1CON1bits.ADON = 1; // turn ADC ON
    AD1CON1bits.ASAM = 1; // auto-sample
    
    /* adc interrupts */
    IFS0bits.AD1IF = 0;
    IEC0bits.AD1IE = 1;
    
    return;
}

void initUart(void){
    TRISBbits.TRISB7 = DIO_OUTPUT;
    ANSBbits.ANSB7 = DIO_DIGITAL;
    
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
    static q16angle_t theta = 0;
    theta += omega;
    
    DAC1DAT = q15_fast_sin(theta) + 32768;
    DAC2DAT = q15_fast_sin(theta + 32768) + 32768; // theta + 180 deg
    
    IFS0bits.T1IF = 0;
    
    return;
}

void _ISR _ADC1Interrupt(void){
    AD1CON1bits.ASAM = 0; // auto-sample off
    
    switch(AD1CHS){
        case LD_VOLTAGE_1_AN:
        {
            /* load voltage 1 (low side) */
            AD1CHS = LD_VOLTAGE_0_AN;
            
            loadVoltageL = (q15_t)(ADC1BUF0 >> 1);
            
            break;
        }

        case LD_VOLTAGE_0_AN:
        {
            /* load voltage 0 (high side) */
            AD1CHS = CURRENT_VOLTAGE_AN;
            
            loadVoltage[sampleIndex] = (q15_t)(ADC1BUF0 >> 1) - loadVoltageL;
            
            break;
        }
        
        case CURRENT_VOLTAGE_AN:
        {
            /* current 1 */
            AD1CHS = HZ_VOLTAGE_1_AN;
            
            loadCurrent[sampleIndex] = (q15_t)(ADC1BUF0 >> 1);
            
            if(++sampleIndex >= NUM_OF_SAMPLES)
                sampleIndex = 0;
            
            break;
        }
        
        case HZ_VOLTAGE_1_AN:
        {
            /* hz out 1 */
            AD1CHS = HZ_OVLTAGE_2_AN;
            
            hz1Voltage = (q15_t)(ADC1BUF0 >> 1);
            
            break;
        }
        
        case HZ_OVLTAGE_2_AN:
        {
            /* hz out 2 */
            AD1CHS = LD_VOLTAGE_1_AN;
            
            hz2Voltage = (q15_t)(ADC1BUF0 >> 1);
            
            break;
        }
        
        default:
        {
            while(1); // programmer's trap
        }
    }
    
    AD1CON1bits.ASAM = 1; // auto-sample
    
    /* clear the flag */
    IFS0bits.AD1IF = 0;
}

