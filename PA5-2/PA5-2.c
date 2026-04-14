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
void interrupt ISR();
unsigned int readADC();
void delay(unsigned int cnt);

// Global variables
volatile unsigned int adcValue = 0;

void main(){
    // Local variables
    unsigned char disp = 0;         // Normalized ADC value
    unsigned char ones, deci;       // BCD temporary variables

    // GPIO ports set-up
    TRISB = 0x00;                   // PORTB as output, 7-segment data
    TRISE = 0x00;                   // PORTE as output, 7-segment control
    TRISA = 0x01;                   // RA0 as input, potentiometer input
    PCFG3 = 1; PCFG2 = 1;           // AN0 as analog (RA0)
        PCFG1 = 1; PCFG0 = 0;       // Vref+ = Vdd, Vref- = Vss

    // A/D module
    CHS2 = 0; CHS1 = 0;             // RA0 channel select (RA0)
        CHS0 = 0;
    ADCS2 = 0; ADCS1 = 0;           // Fosc/8
        ADCS0 = 1;
    ADFM = 1;                       // Right align
    ADON = 1;                       // Turn on AD module

    // Interrupt set-up
    ADIF = 0;
    ADIE = 1;
    PEIE = 1;

    // Initialization
    PORTB = 0x00;
    PORTE = 0x03;
    delay(15);
    GIE = 1;

    // Foreground routine
    while(1){
        // Start conversion
        GO_DONE = 1;

        // Temporarily disable adc interrupt
        ADIE = 0;
        unsigned long tempValue = adcValue * 100UL;
        ADIE = 1;

        // 0.05V = 10.24
        // Divides d_value by 1024 to find 0.05V intervals, adds 1 for rounding, 
        // then divides by 2 to convert to 0.1V steps.
        disp = ((tempValue / 1024) + 1) / 2; 
        
        // Display results
        ones = disp / 10;           // BCD format
        deci = disp % 10;
        
        // Display individually
        // Ones digit
        PORTE = 0x03;
        PORTB = ones;
        PORTE = 0x01;
        delay(2);

        // Decimal digit
        PORTE = 0x03;
        PORTB = deci;
        PORTE = 0x02;
        delay(2);
    }
}

void interrupt ISR(){
    if(ADIE && ADIF){               // When A/D conversion is done
        ADIF = 0;

        // Read results
        adcValue = (ADRESH << 8) | ADRESL;
    }
}

void delay(unsigned int cnt){
    for (int i = 0; i < cnt; i++)
        for (int j = 0; j < 125; j++);
}