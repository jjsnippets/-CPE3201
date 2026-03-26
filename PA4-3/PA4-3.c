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
#define STARTTIME 125                   // Loaded to PR2

// Function prototypes
void interrupt ISR();

// Global variables
volatile int period = 0;

void main(){
    // GPIO ports set-up
    TRISC = 0x04;                       // RC2 for CCP1 input

    // TIMER1 as Capture
    T1CKPS1 = 1; T1CKPS0 = 1;           // 1:8 Prescaler
    CCP1M3 = 0; CCP1M2 = 1;
        CCP1M1 = 0; CCP1M0 = 1;         // Capture mode, every rising edge
    TMR1 = 0;                           // Reset to 0
    CCP1IF = 0;                         // Clear flag
    CCP1IE = 1;                         // Enable TIMER1/CCP1 match interrupt

    // Enable interrupts and timers
    PEIE = 1;                           // Peripheral interrupt
    GIE = 1;                            // Global interrupt
    TMR1ON = 1;                         // TIMER1

    // Wait for interrupts
    while(1);
}

void interrupt ISR(){
    if(CCP1IE && CCP1IF) {              // When rising edge at RC2 (CCP1)
        CCP1IF = 0;
        TMR1 = 0;                       // Reset to 0;

        period = CCPR1/1000;            // Transfers captured TMR1 value
        period *= 8;                    // With normalization


        PORTAbits.RA0 ^= 1;             // Toggle signal

    }
}