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

// Function prototypes
void delay(unsigned int time);

// Frequency (Period) array
// Corresponding to 500Hz, 750Hz, 1000Hz respectively
static unsigned char FREQUENCIES[3] = {
    124, 82, 62
};

// Duty Cycle table
// Row: Frequencies (500Hz, 750Hz, 1000Hz)
// Col: Duty Cycles (10%, 25%, 50%, 75%, 95%)
static unsigned int DUTYCYCLES[3][5] = {
    {50, 125, 250, 375, 475},
    {33,  83, 166, 249, 315},
    {25,  63, 126, 189, 239}
};

// Global variables
//  Software-based rising-edge detector
bit frequencyPState = 0, dutyCyclePState = 0;   // Previous button states
bit frequencyCState = 0, dutyCycleCState = 0;   // Current button states

void main(){
    // Local variables
    unsigned char frequencyCount = 0, dutyCycleCount = 0;
    unsigned int dutyCycle;

    // GPIO ports set-up
    TRISC = 0x00;                       // RC2 for CCP1 output
    ADCON1 = 0x06;                      // PORTE as digital pins
    TRISE = 0x03;                       // RE0 = Frequency cyling
                                        // RE1 = Duty Cycle cycling

    // TIMER2 as PWM
    T2CKPS1 = 1; T2CKPS0 = 0;           // 1:16 prescaler
    CCP1M3 = 1; CCP1M2 = 1;             // PWM mode
        CCP1M1 = 0; CCP1M0 = 0;

    // Enable timers
    TMR2ON = 1;                         // TIMER2

    // Foreground loop
    while(1){
        // Update Frequency state
        frequencyCState = RE0;
        if(frequencyCState && !frequencyPState)
            frequencyCount = (frequencyCount < 2) ? frequencyCount + 1 : 0;
        frequencyPState = frequencyCState;

        // Update Duty Cycle state
        dutyCycleCState = RE1;
        if(dutyCycleCState && !dutyCyclePState)
            dutyCycleCount = (dutyCycleCount < 4) ? dutyCycleCount + 1 : 0;
        dutyCyclePState = dutyCycleCState;

        // Update PWM registers
        PR2 = FREQUENCIES[frequencyCount];
        
        dutyCycle = DUTYCYCLES[frequencyCount][dutyCycleCount];
        CCPR1 = (dutyCycle >> 2) & 0xFF;
        CCP1X = (dutyCycle >> 1) & 1;
        CCP1Y = (dutyCycle) & 1;

        delay(100);
    }
}

// Software-defined delay
void delay(unsigned int time){
    for (int i = 0; i < time; i++)
        for (int j = 0; j < 125; j++);
}