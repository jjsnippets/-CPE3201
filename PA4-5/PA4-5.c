// CPE 3201 - EMBEDDED SYSTEMS
// Group 4      TTh 7:30 AM - 10:30 AM LB 285 TC
// Sarcol, Joshua S.        BS CpE - 3      2026/03/26
// Practical Activity 4: Timers (Timer1, Timer2 and CCP Module) 

// Microcontroler files
#include <xc.h>
#include <pic16f877a.h>

// Configuration bits
#pragma config FOSC = XT
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

// Constants
#define PERIOD 124                      // Loaded to PR2
#define DUTYCYCLE 350                   // Loaded to CCPR1, CCP1X, and CCP1Y

// Global variables
volatile int period = 0;

void main(){
    // GPIO ports set-up
    TRISC = 0x00;                       // RC2 for CCP1 output

    // TIMER2 as PWM
    T2CKPS1 = 1; T2CKPS0 = 0;           // 1:16 prescaler
    CCP1M3 = 1; CCP1M2 = 1;             // PWM mode
        CCP1M1 = 0; CCP1M0 = 0;         
    PR2 = PERIOD;                       // ~0.002 s
    CCPR1 = (DUTYCYCLE >> 2) & 0xFF;    // 70% duty cycle
        CCP1X = (DUTYCYCLE >> 1) & 1;
        CCP1Y = (DUTYCYCLE) & 1;        

    // Initialization
    PORTAbits.RA0 = 0;                  // LOW signal

    // Enable timers
    TMR2ON = 1;                         // TIMER2

    // Generate PWM signals forever
    while(1);
}