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

void main(){
    // GPIO ports
    TRISB = 0x00; PORTB = 0x00; // PORTB as output

    // USART Asynchronous Reception
    SPEN = 1;                   // Serial port enable
    SYNC = 0;                   // Asynchronous mode
    BRGH = 1;                   // High speed
    SPBRG = 0x19;               // 9.6Kbaud at 4MH; 0.16% error
    RX9 = 0;                    // 8-bit reception

    // Foreground routine
    CREN = 1;                   // Start of continuous reception
    while(1) {
        while(!RCIF);           // When RXREG is full
        PORTB = RCREG;          // Then accept data (directly to PORTB)
    }
}
