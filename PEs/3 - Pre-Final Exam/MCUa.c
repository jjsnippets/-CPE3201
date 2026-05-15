
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

#define _XTAL_FREQ      4000000UL

// Function prototypes
unsigned int keypadScan();
unsigned int readADC();

void main(){
    // Local variables
    unsigned char prev_input, cur_input = 99;
    unsigned long adcValue = 0;
    unsigned char sendValue = 0;
    unsigned char isCounting = 1;

    // Set GPIO ports
    TRISA = 0x01;               // RA0 as input, potentiometer input
    TRISD = 0x0F;               // Upper nibble of PORTD as output
                                // Lower nibble of PORTD as input
    
    // A/D module
    PCFG3 = 1; PCFG2 = 1;           // AN0 as analog (RA0)
        PCFG1 = 1; PCFG0 = 0;       // Vref+ = Vdd, Vref- = Vss
    CHS2 = 0; CHS1 = 0;             // RA0 channel select (RA0)
        CHS0 = 0;
    ADCS2 = 0; ADCS1 = 0;           // Fosc/8
        ADCS0 = 1;
    ADFM = 1;                       // Right align
    ADON = 1;                       // Turn on AD module
    
    // USART Asynchronous Transmission
    SPEN = 1;           // Serial port enable
    SYNC = 0;           // Asynchronous mode
    BRGH = 1;           // High speed
    SPBRG = 0x19;       // 9.6Kbaud at 4MH; 0.16% error
    TX9 = 0;            // 8-bit transmission

    // Foreground routine
    TXEN = 1;           // Start of transmission
    while(1) {
        while(cur_input == 99)
           cur_input = keypadScan();   // When a keypad button is pressed

        switch(cur_input){
            case '*':       // Send value
                adcValue = readADC() * 100UL;       // Grab value from ADC

                // 0.05V = 10.24
                // Divides adcValue by 1024 to find 0.05V intervals, adds 1 for rounding, 
                // then divides by 2 to convert to 0.1V steps.
                sendValue = (((adcValue / 1024) + 1) / 2); 

                while(!TRMT);           // When TSR is empty (ready to send)
                TXREG = sendValue;      // Then load a value to be sent

                break;
            /*
            case '#':       // Toggle counting
                if(isCounting){
                    isCounting = 0;         // Toggle to pause
                    while(!TRMT);           // When TSR is empty (ready to send)
                    TXREG = 99;             // Then send pause signal
                } else {
                    isCounting = 1;         // Toggle to start counting
                    while(!TRMT);           // When TSR is empty (ready to send)
                    TXREG = sendValue;      // Then send displayed value
                }
                break;
            */
            default:        // Do/change nothing otherwise
                break;
        }

        prev_input = cur_input;         // Software debouncing implementation
        while(prev_input == cur_input)
            cur_input = keypadScan();
    }
}

// Non-interrupt keypad scanning implementation
unsigned int keypadScan(){
    unsigned char input;
    PORTD = 0x00;

    while (1){
        // First column
        PORTD = 0x10;
        __delay_ms(5);
        input = PORTD & 0x0F;
        switch (input){
            case 0x01:
                return '1';
            case 0x02:
                return '4';
            case 0x04:
                return '7';
            case 0x08:
                return '*';
        }

        // Second column
        PORTD = 0x20;
        __delay_ms(5);
        input = PORTD & 0x0F;
        switch (input){
            case 0x01:
                return '2';
            case 0x02:
                return '5';
            case 0x04:
                return '8';
            case 0x08:
                return '0';
        }

        // Third column
        PORTD = 0x40;
        __delay_ms(5);
        input = PORTD & 0x0F;
        switch (input){
            case 0x01:
                return '3';
            case 0x02:
                return '6';
            case 0x04:
                return '9';
            case 0x08:
                return '#';
        }

        // Otherwise send "nothing"
        return 99;
    }

    return 0;       // Suppress compiler warnings
}

unsigned int readADC(){
    unsigned int res = 0;       // Result variable
    __delay_ms(15);             // Aquisition time
    GO_DONE = 1;                // Start conversion
    while(GO_DONE);             // Wait for conversion to finish

    // Read results
    res = (ADRESH << 8) | ADRESL;

    return res;
}