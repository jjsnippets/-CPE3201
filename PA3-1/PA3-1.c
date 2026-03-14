// CPE 3201 - EMBEDDED SYSTEMS
// Group 4      TTh 7:30 AM - 10:30 AM LB 285 TC
// Sarcol, Joshua S.        BS CpE - 3      2026/03/13
// Practical Activity 3: Interrupts (External and Timer Overflows)

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
    '1', '2', '3', 0x0F,
    '4', '5', '6', 0x0F,
    '7', '8', '9', 0x0F,
    0x0F, '0', 0x0F, 0x0F,
};

void main(){
    // Local variables

    // GPIO ports set-up
    TRISB = 0x01;       // RB0 as input (as interrupt)
    TRISC = 0x00;       // Lower nibble of PORTC as output (7-segment)
    TRISD = 0x0F;       // Lower nibble of PORTD as input (keypad)

    // Interrupt set-up
    // RB0 as interrupt
    INTEDG = 1;         // At rising edge
    INTF = 0;           // Clear flag
    INTE = 1;           // Enable RB0 interrupt

    GIE = 1;            // Enable all interrupts

    // 7-segment initialization
    PORTC = 0x00;

    // Nothing loop; to be interrupted
    while(1);
}

void interrupt ISR(){
    GIE = 0;

    if(INTF){                           // If RB0 is triggered
        INTF = 0;                       // Clear RB0 flag

        PORTC = KEYPAD[PORTD & 0x0F];   // From keypad to 7-segment
    }

    GIE = 1;
}