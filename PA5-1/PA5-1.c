// CPE 3201 - EMBEDDED SYSTEMS
// Group 4      TTh 7:30 AM - 10:30 AM LB 285 TC
// Sarcol, Joshua S.        BS CpE - 3      2026/04/08
// Practical Activity 5: Analog to Digital Converter

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
unsigned int readADC();
void delay(unsigned int cnt);

// AD comparison array
static unsigned int ADCOMP[] = {170, 341, 512, 683, 854, 1025};

// LEDs display array
static unsigned char LEDS[] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F};

void main(){
    // Local variables
    unsigned int d_value = 0;
    unsigned char ledResult = 0;

    // GPIO ports set-up
    TRISB = 0x00;               // PORTB as output, LEDs
    TRISA = 0x01;               // RA0 as input, potentiometer input
    PCFG3 = 1; PCFG2 = 1;       // AN0 as analog (RA0)
        PCFG1 = 1; PCFG0 = 0;   // Vref+ = Vdd, Vref- = Vss

    // A/D module
    CHS2 = 0; CHS1 = 0;         // RA0 channel select (RA0)
        CHS0 = 0;
    ADCS2 = 0; ADCS1 = 0;       // Fosc/8
        ADCS0 = 1;
    ADFM = 1;                   // Right align
    ADON = 1;                   // Turn on AD module

    // Initialize to 0
    PORTB = 0x00;

    // Foreground routine
    while(1){
        d_value = readADC();    // Get ADC value
        ledResult = 0;          // Reset LED indicators to 0

        // Repeatedly increase comparison value based on precalculated array
        while(1)
            if(ADCOMP[ledResult] <= d_value) ledResult++;
            else break;
        
        // Display results
        PORTB = LEDS[ledResult];
    }
}

unsigned int readADC(){
    unsigned int res = 0;       // Result variable
    delay(15);                  // Aquisition time
    GO_DONE = 1;                // Start conversion
    while(GO_DONE);             // Wait for conversion to finish

    // Read results
    res = (ADRESH << 8) | ADRESL;

    return res;
}

void delay(unsigned int cnt){
    for (int i = 0; i < cnt; i++)
        for (int j = 0; j < 125; j++);
}