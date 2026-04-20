// CPE 3201 - EMBEDDED SYSTEMS
// Group 4      TTh 7:30 AM - 10:30 AM LB 285 TC
// Sarcol, Joshua S.        BS CpE - 3      2026/04/20
// Practical Activity 6: Universal Synchronous Asynchronous Receiver Transmitter (USART)

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

// Keypad array
static unsigned char KEYPAD[] = {
    0x01, 0x02, 0x03, 0x0E,
    0x04, 0x05, 0x06, 0x0E,
    0x07, 0x08, 0x09, 0x0E,
    0x0F, 0x00, 0x0F, 0x0E,
};

// Global variables
volatile bit sendInput = 0;
volatile unsigned char input = '0';

void main(){
    // GPIO ports
    TRISB = 0xF1;       // RB0 input interrupt
                        // Upper nibble of PORTB as input from keypad

    // RB0 interrupt
    INTEDG = 1;         // At rising edge
    INTF = 0;           // Clear flag
    INTE = 1;           // Enable RB0 interrupt

    // USART Asynchronous Transmission
    SPEN = 1;           // Serial port enable
    SYNC = 0;           // Asynchronous mode
    BRGH = 1;           // High speed
    SPBRG = 0x19;       // 9.6Kbaud at 4MH; 0.16% error
    TX9 = 0;            // 8-bit transmission

    // Foreground routine
    GIE = 1;            // Enable all interrupts
    TXEN = 1;           // Start of transmission
    while(1){
        // After accepting keypad press
        if(sendInput){
            while(!TXIF);   // Wait until TXREG is empty
            TXREG = input;  // Then send the keypad input

            // Finished transmission
            sendInput = 0; 
        }
    }
}

void interrupt ISR(){
    if(INTE && INTF){                               // When keypad is pressed
        INTF = 0;
        if(!sendInput){                             // Drop if still sending string
            input = KEYPAD[(PORTB & 0xF0) >> 4];    // Accept number from keypad
            sendInput = 1;                          // Then start sending input
        }
    }
}