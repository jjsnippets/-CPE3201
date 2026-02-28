// CPE 3201 - EMBEDDED SYSTEMS
// Group 4      TTh 7:30 AM - 10:30 AM LB 285 TC
// Sarcol, Joshua S.        BS CpE - 3      2026/02/28
// Practical Activity 2: IO Configuration and Control

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
void initLCD();
void instCtrl(unsigned char INST);
void dataCtrl(unsigned char DATA);
void delay(unsigned int x);

void main(){
    // Set GPIO ports
    TRISB = 0x00;       // PORTB as output
    TRISC = 0x00;       // PORTC as output
    
    // LCD initialization
    initLCD();
    instCtrl(0xC6);     // 2nd line, 7th column

    // Send "HELLO!" one at a time
    dataCtrl('H');
    dataCtrl('E');
    dataCtrl('L');
    dataCtrl('L');
    dataCtrl('O');
    dataCtrl('!');

    // Stop further execution
    while(1);
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
    PORTCbits.RC0 = 0;  // RS = 0   Instruction byte
    PORTCbits.RC1 = 1;  // E = 1    Enable
    delay(15);          //          Delay
    PORTCbits.RC1 = 0;  // E = 0    Disable
}

// Helper function to send a data byte to LCD
void dataCtrl(unsigned char DATA){
    PORTB = DATA;       //          Send to LCD
    PORTCbits.RC0 = 1;  // RS = 1   Data byte
    PORTCbits.RC1 = 1;  // E = 1    Enable
    delay(15);          //          Delay
    PORTCbits.RC1 = 0;  // E = 0    Disable
}

// Software-defined delay
void delay(unsigned int time){
    for (int i = 0; i < time; i++)
        for (int j = 0; j < 25; j++);
}