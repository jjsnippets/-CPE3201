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
void interrupt ISR();
void delay(unsigned int cnt);

// Keypad array
static unsigned char KEYPAD[] = {
    '1', '2', '3', 0x0F,
    '4', '5', '6', 0x0F,
    '7', '8', '9', 0x0F,
    0x0F, '0', 0x0F, 0x0F,
};

// Global variables
bit ledStatus = 0;
bit count_flag = 0;

void main(){
    // GPIO ports set-up
    TRISA = 0x00;                       // RA0 as output (Blinking LED)
    TRISB = 0x01;                       // RB0 as input (as interrupt)
    TRISC = 0x00;                       // Lower nibble of PORTC as output (7-segment)
    TRISD = 0x0F;                       // Lower nibble of PORTD as input (keypad)

    // Interrupt set-up
    // RB0 as interrupt
    INTEDG = 1;                         // At rising edge
    INTF = 0;                           // Clear flag
    INTE = 1;                           // Enable RB0 interrupt

    // TMR0
    PS2 = 1; PS1 = 0; PS0 = 0;          // 1:32 Prescaler
    PSA = 0;                            // Presacler to TMR0
    T0CS = 0;                           // Internal instruction clock cycle
    T0IF = 0;                           // Clear flag
    T0IE = 1;                           // Enable TMR0 interrupt

    GIE = 1;                            // Enable all interrupts

    // Initialization to 0
    PORTC = 0x00;
    PORTAbits.RA0 = ledStatus;

    // Foreground loop
    while(1) {
        delay(122);                     // Wait for 1 second
        ledStatus ^= 1;                 // Flip state of ledStatus
        PORTAbits.RA0 = ledStatus;      // Blink LED
    }
}

void interrupt ISR(){
    GIE = 0;

    if(INTF) {                          // If RB0 is triggered
        INTF = 0;                       // Clear RB0 flag

        PORTC = KEYPAD[PORTD & 0x0F];   // From keypad to 7-segment

    } else
    if(T0IF) {                          // If TMR0 is overflowed
        T0IF = 0;

        count_flag = 1;                 // Indicate TMR0 overflow for delay()
    
    }

    GIE = 1;
}

void delay(unsigned int cnt){
    for (int i = 0; i < cnt; i++){
        while(!count_flag);             // Wait until TMR0 overflows
        count_flag = 0;                 // Wait again cnt times
    }
}