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

// Keypad array
static unsigned char KEYPAD[] = {
    '1', '2', '3', 'X',
    '4', '5', '6', 'X',
    '7', '8', '9', 'X',
    '*', '0', '#', 'X',
};

// LCD DDRAM Address Locations
typedef enum LINES {
    LINE1 = 0x80,
    LINE2 = 0xC0,
    LINE3 = 0x94,
    LINE4 = 0xD4
} LINES;

void main(){
    // Set GPIO ports
    TRISB = 0x00;               // PORTB as output
    TRISC = 0x00;               // PORTC as output
    TRISD = 0xFF;               // PORTD as input

    // Local variables
    unsigned char input;    
    unsigned int numChars = 0;  // Number of characters displayed in the screen

    // LCD initialization
    initLCD();
    instCtrl(LINE1);            // To top-left corner

    // Main program loop
    while(1){
        if(PORTDbits.RD4){                  // When a keypad button is pressed
            input = PORTD & 0x0F;           // Then grab keypad data
            dataCtrl(KEYPAD[input]);        // Then display to LCD
            switch (++numChars){            // Update number of characters in LCD variable
            case 20:                            // When 20 characters are sent
                instCtrl(LINE2);                // Then advance to second line
                break;
            case 40:                            // When 40 characters are sent
                instCtrl(LINE3);                // Then advance to third line
                break;
            case 60:                            // When 60 characters are sent
                instCtrl(LINE4);                // Then advance to fourth line
                break;
            case 80:                            // When 80 lines are sent
                instCtrl(0x01);                 // Then clear screen full of characters
                instCtrl(LINE1);                // Reset to top-left corner
                dataCtrl(KEYPAD[input]);        // Resend cleared character
                numChars = 1;                   // Update number of characters in LCD variable
                break;
            }
            while(PORTDbits.RD4);           // Wait until keypad button is depressed
        }
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