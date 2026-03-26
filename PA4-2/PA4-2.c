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

void main(){
    // GPIO ports set-up
    TRISA = 0x00;                       // PORTA as output
    ADCON1 = 0x06;                      // PORTA as digital I/O

    // TIMER2
    T2CKPS1 = 0; T2CKPS0 = 1;           // 1:4 Clock prescaler
    PR2 = STARTTIME;                    // Loaded time for TIMER2
    TMR2IF = 0;                         // Clear flag
    TMR2IE = 1;                         // Enable TIMER1 interrupt

    // Initialization
    PORTAbits.RA0 = 0;                  // LED off

    // Enable interrupts and timers
    PEIE = 1;                           // Peripheral interrupt
    GIE = 1;                            // Global interrupt
    TMR2ON = 1;                         // TIMER2

    // Wait for interrupts
    while(1);
}

void interrupt ISR(){
    if(TMR2IE && TMR2IF) {              // Every 0.5s (from TIMER1) 
        TMR2IF = 0;

        PORTAbits.RA0 ^= 1;             // Toggle signal

    }
}