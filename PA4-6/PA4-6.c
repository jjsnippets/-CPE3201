// CPE 3201 - EMBEDDED SYSTEMS
// Group 4      TTh 7:30 AM - 10:30 AM LB 285 TC
// Sarcol, Joshua S.        BS CpE - 3      2026/03/30
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
void interrupt ISR();
void initLCD();
void instCtrl(unsigned char INST);
void dataCtrl(unsigned char DATA);
void delay(unsigned int time);

// LCD DDRAM Address Locations
typedef enum LINES {
    LINE1 = 0x80,
    LINE2 = 0xC0,
    LINE3 = 0x94,
    LINE4 = 0xD4
} LINES;

// Global variables
//  Maximum for 16-bit register: 65,535 * 8 us = 524.28 ms
//  Which is less than the 1000 ms requirement
//  So needs to be expanded to 32 bits
volatile unsigned char overflowCount = 0;
volatile unsigned long totalIncrements = 0;

void main(){
    // Local variables
    unsigned int p3, p2, p1, p0;        // Place values to be displayed

    // GPIO ports set-up
    TRISB = 0x00;                       // PORTB as output to LCD
    TRISC = 0x04;                       // RC2 for CCP1 input
    ADCON1 = 0x06;                      // PORTE as digital pins
    TRISE = 0x00;                       // LCD Control Lines
                                        // RE0 as RS; RE1 as E

    // TIMER1 as Capture
    T1CKPS1 = 1; T1CKPS0 = 1;           // 1:8 Prescaler
    CCP1M3 = 0; CCP1M2 = 1;             // Capture mode, every rising edge
        CCP1M1 = 0; CCP1M0 = 1;         
    TMR1 = 0;                           // Reset to 0
    TMR1IF = 0; CCP1IF = 0;             // Clear flags
    CCP1IE = 1;                         // Enable TIMER1/CCP1 match interrupt

    // LCD Initialization
    initLCD();
    instCtrl(LINE1);                    // To top-left corner
    for(int i = 0; i < 5; i++)
        dataCtrl(' ');                  // 5 blank spaces for the digit
    dataCtrl('m');                      // xxxx ms
    dataCtrl('s');
    instCtrl(LINE1);                    // Reset position

    // Enable interrupts and timers
    PEIE = 1;                           // Peripheral interrupt
    GIE = 1;                            // Global interrupt
    TMR1IE = 1;                         // TIMER1 overflow
    TMR1ON = 1;                         // TIMER1

    // Foreground routine
    // Displaying to LCD current input frequency
    while(1) {
        // Quickly disable capture interrupt
        CCP1IE = 0;
        unsigned long tempIncrements = totalIncrements;
        CCP1IE = 1;

        // Transfers captured TMR1 value in microseconds
        unsigned int period = ((tempIncrements * 8) + 500) / 1000;

        // Extracting individual digits
        // Min: 1000 Hz =    1 ms
        // Max:    1 Hz = 1000 ms
        p0 = period % 10;               
        p1 = (period % 100) / 10;
        p2 = (period % 1000) / 100;
        p3 = period / 1000;

        // Display number
        if(!p3)                         // If thousands place is 0
            dataCtrl(' ');              // Then do not display thousands place
        else
            dataCtrl(p3 + '0');

        if(!p3 && !p2)                  // And if hundreds place is 0
            dataCtrl(' ');              // Then do not also display hundreds place
        else
            dataCtrl(p2 + '0');

        if(!p3 && !p2 && !p1)           // And if tens place is 0
            dataCtrl(' ');              // Then do not also display tens place
        else
            dataCtrl(p1 + '0');

            dataCtrl(p0 + '0');         // Display ones place no matter what

        instCtrl(LINE1);                // Reset position
    }
}

void interrupt ISR(){
    if(TMR1IE && TMR1IF) {              // When timer 1 overflows
        TMR1IF = 0;
        overflowCount++;                // Then keep track of overflows
    }

    if(CCP1IE && CCP1IF) {              // When rising edge at RC2 (CCP1)
        CCP1IF = 0;
        
        // Count total increments (Expand to 32-bit result)
        totalIncrements = ((unsigned long) overflowCount << 16) | CCPR1;

        overflowCount = 0; TMR1 = 0;    // Reset to 0
    }
}

// Helper function to initialize the LCD
void initLCD(){
    delay(15);          // ~15ms start-up time
    
    instCtrl(0x38);     // 0011 1000 0x38
                        // 001? ??xx Function set
                        // ---1 ---- 8-bit data transfer
                        // ---- 1--- Dual line display
                        // ---- -0-- 5x7 font size
    
    instCtrl(0x08);     // 0000 1000 0x08
                        // 0000 1??? Display ON/OFF
                        // ---- -0-- Entire display off
                        // ---- --0- Cursor off
                        // ---- ---0 Cursor blinking off
    
    instCtrl(0x01);     // 0000 0001 0x01
                        // 0000 0001 Clear display
    
    instCtrl(0x06);     // 0000 0110 0x06
                        // 0000 01?? Entry mode set
                        // ---- --1- Increment / Move right
                        // ---- ---0 No shifting

    instCtrl(0x0E);     // 0000 1110 0x0E
                        // 0000 1??? Display ON/OFF
                        // ---- -1-- Entire display on
                        // ---- --1- Cursor on
                        // ---- ---0 Cursor blinking off 
}

// Helper function to send an instruction byte to LCD
void instCtrl(unsigned char INST){
    PORTB = INST;       //          Send to LCD
    RE0 = 0;            // RS = 0   Instruction byte
    RE1 = 1;            // E = 1    Enable
    delay(15);          //          Delay
    RE1 = 0;            // E = 0    Disable
}

// Helper function to send a data byte to LCD
void dataCtrl(unsigned char DATA){
    PORTB = DATA;       //          Send to LCD
    RE0 = 1;            // RS = 1   Data byte
    RE1 = 1;            // E = 1    Enable
    delay(15);          //          Delay
    RE1 = 0;            // E = 0    Disable
}

// Software-defined delay
void delay(unsigned int time){
    for (int i = 0; i < time; i++)
        for (int j = 0; j < 125; j++);
}