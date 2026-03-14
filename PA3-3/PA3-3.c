// CPE 3201 - EMBEDDED SYSTEMS
// Group 4      TTh 7:30 AM - 10:30 AM LB 285 TC
// Sarcol, Joshua S.        BS CpE - 3      2026/03/14
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
void delay(unsigned int cnt);

// Keypad array
static unsigned char KEYPAD[] = {
    0x01, 0x02, 0x03, 0x0F,
    0x04, 0x05, 0x06, 0x0F,
    0x07, 0x08, 0x09, 0x0F,
    0x0F, 0x00, 0x0F, 0x0F,
};

// Global variables
unsigned int counter = 0;               // Current count displayed in 7-segment
unsigned char input = 0;                // Accepts keypad input
bit timerOverflow = 0;                  // Indicates TMR0 overflow

void main(){
    // GPIO ports set-up
    TRISB = 0x01;                       // RB0 as input (as interrupt)
    TRISC = 0x00;                       // Lower nibble of PORTC as output (7-segment)
    TRISD = 0x0F;                       // Lower nibble of PORTD as input (keypad)

    // Interrupt set-up
    // RB0 as interrupt
    INTEDG = 1;                         // At rising edge
    INTF = 0;                           // Clear flag
    INTE = 1;                           // Enable RB0 interrupt

    // TMR0
    PS2 = 1; PS1 = 0; PS0 = 1;          // 1:64 Prescaler
    PSA = 0;                            // Presacler to TMR0
    T0CS = 0;                           // Internal instruction clock cycle
    T0IF = 0;                           // Clear flag
    T0IE = 1;                           // Enable TMR0 interrupt

    // Initialization to 0
    PORTC = counter;

    // Enable all interrupts
    GIE = 1;                            

    // Foreground loop
    // Deals with counter incrementation
    while(1) {
        delay(1);                       // Wait for 0.8 seconds

        if(counter >= 0x09)             // If about to increment past 9
            counter = 0x00;             // Then reset to 0
        else                            // Otherwise
            counter++;                  // Proceed to next number
        
        PORTC = counter;                // Display incremented number
    }
}

void interrupt ISR(){
    GIE = 0;

    if(INTF) {                          // If RB0 is triggered
        INTF = 0;                       // Clear RB0 flag

        // Deals with the keypress update
        // Mapping from keypress to 7-segment display
        input = KEYPAD[PORTD & 0x0F];

        // If valid mapping, then change the displayed number
        // Otherwise, ignore update
        counter = (input == 0x0F) ? counter : input;
        PORTC = counter;

    } else if(T0IF) {                   // If TMR0 is overflowed
        T0IF = 0;                       // Clear TMR0 flag

        timerOverflow = 1;              // Indicate TMR0 overflow for delay()
    
    }

    GIE = 1;
}

void delay(unsigned int cnt){
    unsigned int u_cnt = cnt * 49;      // 1 cnt = 0.80282 seconds
    for (int i = 0; i < u_cnt; i++){
        while(!timerOverflow);          // Wait until TMR0 overflows
        timerOverflow = 0;              // Wait again u_cnt times
    }
}