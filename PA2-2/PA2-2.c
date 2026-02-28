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

// Keypad array
static unsigned char KEYPAD[] = {
    0x01, 0x02, 0x03, 0x0F,
    0x04, 0x05, 0x06, 0x0F,
    0x07, 0x08, 0x09, 0x0F,
    0x0F, 0x00, 0x0F, 0x0F,
};

void main(){
    // Set GPIO ports
    ADCON1 = 0x06;                  // PORTA as digital pins
    TRISA = 0x00;                   // PORTA as output
    TRISD = 0xFF;                   // PORTD as input
    
    // Clear all outputs
    PORTA = 0x00;       
    
    // Local variables
    unsigned char input;
    
    // Main program loop
    while(1){
        if(PORTDbits.RD4){          // When keypad is pressed
            input = PORTD & 0x0F;   // Then grab keypad data
            PORTA = KEYPAD[input];  // Then display to LEDs
            while(PORTDbits.RD4);   // Wait until keypad is depressed
        }
    }
}