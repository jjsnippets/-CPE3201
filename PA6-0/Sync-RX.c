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
    // GPIO ports
    TRISB = 0x00; PORTB = 0x00; // PORTB as output

    // USART Syncronous Master Reception
    SPEN = 1;                   // Serial port enable
    SYNC = 1;                   // Syncronous mode
    BRGH = 0;                   // Low speed (ignored)
    SPBRG = 0x67;               // 9.6kBaud at 4Mhz; 0.16% error
    CSRC = 1;                   // Master mode (internal clock)
    RCIE = 1;                   // Enable RX interrupt
    RX9 = 0;                    // 8-bit transmission

    // Enable interrupts
    PEIE = 1;
    GIE = 1;

    // Main program loop
    CREN = 1;                   // Start of continuous reception
    while(1) {
        if (OERR) {             // If overrun error detected
            CREN = 0;           // Toggle CREN to clear OERR
            PORTB = RCREG;      // Clear two bytes in RCREG
            PORTB = RCREG;
            CREN = 1;           // Re-enable transmission
        }
    }
}

void interrupt ISR(){
    if(RCIE && RCIF){           // When there is data recieved
        PORTB = RCREG;          // Then accept data (directly to PORTB)
    }
}