// CPE 3201 - EMBEDDED SYSTEMS
// Group 4      TTh 7:30 AM - 10:30 AM LB 285 TC
// Sarcol, Joshua S.        BS CpE - 3      2026/04/18
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
    '1', '2', '3', 'X',
    '4', '5', '6', 'X',
    '7', '8', '9', 'X',
    '*', '0', '#', 'X',
};

// Global variables
volatile bit sendString = 0;
volatile unsigned char input = '0';
const char* string = "You pressed ";

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
        if(sendString){         
            // Then send the entire string first
            for(char i = 0; string[i] != '\0'; i++){
                while(!TXIF);       // Wait until TXREG is empty
                TXREG = string[i];  // if so, send the string character-by-character
            }

            // Send individual characters (unrolled)
            while(!TXIF);   TXREG = input;
            while(!TXIF);   TXREG = '.';
            while(!TXIF);   TXREG = '\r';
            while(!TXIF);   TXREG = '\n';

            // Then indicate finished sending string
            sendString = 0;
        }
    }
}

void interrupt ISR(){
    if(INTE && INTF){                               // When keypad is pressed
        INTF = 0;
        if(!sendString){                            // Drop if still sending string
            input = KEYPAD[(PORTB & 0xF0) >> 4];    // Accept number from keypad
            sendString = 1;                         // Then start sending string
        }
    }
}
