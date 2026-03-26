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
#define PERIOD (0xFFFF - 62500)         // Load to TMR1

// Function prototypes
void interrupt ISR();

void main(){
    // GPIO ports set-up
    TRISA = 0x00;                       // PORTA as output
    ADCON1 = 0x06;                      // PORTA as digital I/O

    // TIMER1
    T1CKPS1 = 1; T1CKPS0 = 1;           // 1:8 Prescaler
    T1OSCEN = 0;                        // TIMER1 oscillator disabled
    TMR1CS = 0;                         // Internal clock
    TMR1 = PERIOD;                      // ~0.5 s
    TMR1IF = 0;                         // Clear flag
    TMR1IE = 1;                         // Enable TIMER1 interrupt

    // Initialization
    PORTAbits.RA0 = 0;                  // LED off

    // Enable interrupts and timers
    PEIE = 1;                           // Peripheral interrupt
    GIE = 1;                            // Global interrupt
    TMR1ON = 1;                         // TIMER1

    // Wait for interrupts
    while(1);
}

void interrupt ISR(){
    if(TMR1IE && TMR1IF) {              // Every 0.5 s (from TIMER1) 
        TMR1IF = 0;

        TMR1 = PERIOD;                  // Then re-set TIMER1
        PORTAbits.RA0 ^= 1;             // Toggle LED

    }
}