// CPE 3201 - EMBEDDED SYSTEMS
// Group 4      TTh 7:30 AM - 10:30 AM LB 285 TC
// Sarcol, Joshua S.        BS CpE - 3      2026/04/17
// Seatwork: Synchronous Transmit and Receive

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

void main(){
    // USART Syncronous Master Transmission
    SPEN = 1;               // Serial port enable
    SYNC = 1;               // Syncronous mode
    BRGH = 0;               // Low speed (ignored)
    SPBRG = 0x67;           // 9.6kBaud at 4Mhz; 0.16% error
    CSRC = 1;               // Master mode (internal clock)
    TXIE = 1;               // Enable TX interrupt
    TX9 = 0;                // 8-bit transmission

    // Enable interrupts
    PEIE = 1;
    GIE = 1;

    // Main program loop
    TXEN = 1;               // Start of transmission
    while(1);               // Wait until TXREG is empty
                            // Then continously send 'A's
}

void interrupt ISR(){
    if(TXIE && TXIF){       // When TXREG is empty
        TXREG = 'A';        // Then load a value of 'A' to be sent
    }
}