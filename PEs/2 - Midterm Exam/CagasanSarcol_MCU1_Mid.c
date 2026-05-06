// CPE 3201 - EMBEDDED SYSTEMS
// Group 4      TTh 7:30 AM - 10:30 AM LB 285 TC
// (T2) Cagasan, Mar Joseph V.; Sarcol, Joshua S.         BS CpE - 3      2026/05/05
// Midterm Examination: Practical Exam (CO2) - 2026

// Create a code that will function according to the circuit functionality shown by the instructor. You may download the Proteus project file here.
// Kindly take note of the following specifications:
// - Potentiometer shall be used to vary the input voltage to MCU1.
// - LCD shall display the input voltage value from the potentiometer to the MCU1.
// - Push button shall trigger the sending of voltage data from MCU1 to MCU2.
// - 7 segment displays shall show the voltage value received by MCU2 from MCU1.
// - Oscillator frequency used in the two MCUs is 4MHz.
//
// Submit your C files using the filename format:
//      For MCU1: FamilyNames_MCU1_Mid.c
//      For MCU2: FamilyNames_MCU2_Mid.c

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

// Hardware configuration
#define _XTAL_FREQ 4000000

// LCD DDRAM Address Locations
typedef enum LINES {
    LINE1 = 0x80,
    LINE2 = 0xC0,
    LINE3 = 0x94,
    LINE4 = 0xD4
} LINES;

// Function prototypes
void inline initLCD();
void inline instCtrl(unsigned char inst);
void inline dataCtrl(unsigned char data);
void inline dataStr(const char* string);

// Global variables
volatile unsigned int adcValue = 0;
volatile unsigned char sendValue = 0;
volatile bit sendSignal = 0;

void main() {
    // Local variables
    unsigned char disp = 0;         // Normalized ADC value
    unsigned char ones, deci;       // BCD temporary variables

    // GPIO ports set-up
    TRISD = 0x00; PORTD = 0x00;     // Output: LCD data lines
    TRISB = 0x01;                   // xxxx oooi (0x01)
                                    // ---- SWE- Output: LCD control lines
                                    // ---- ---I Input: RB0/INT button interrupt
    TRISA = 0x0E;                   // xxxx iiix (0x0E)
                                    // ---- p--- Vref+ (+9V)
                                    // ---- -n-- Vref- (0V)
                                    // ---- --A- Analog Input: Potentiometer

    // A/D Module set-up
    PCFG3 = 1; PCFG2 = 0;           // All analog ports
        PCFG1 = 0; PCFG0 = 0;       // Vref+ = AN3/RA3, Vref- = AN2/RA2
    CHS2 = 0; CHS1 = 0;             // RA1 channel select
        CHS0 = 1;
    ADCS2 = 0; ADCS1 = 0;           // Fosc/8
        ADCS0 = 1;
    ADFM = 1;                       // Right align
    ADON = 1;                       // Turn on AD module
    ADIE = 1; ADIF = 0;             // Enable AD interrupt
    
    // RB0/INT set-up
    INTEDG = 1;                     // At rising edge
    INTE = 1;   INTF = 0;           // Enable RB0 interrupt

    // USART Asynchronous Transmission
    SPEN = 1;                       // Serial port enable
    SYNC = 0;                       // Asynchronous mode
    BRGH = 1;                       // High speed
    SPBRG = 0x19;                   // 9.6Kbaud at 4MH; 0.16% error
    TX9 = 0;                        // 8-bit transmission

    // LCD initialization
    initLCD();
    instCtrl(LINE1);
    dataStr("Voltage: ");
    instCtrl(LINE2);
    dataStr("0.0");

    // Interrupts
    PEIE = 1;   GIE = 1;            // Enable interrupts
    TXEN = 1;                       // Start of transmission
    
    // Main program loop
    while(1){
        // Start conversion
        GO_DONE = 1;

        // Temporarily disable adc interrupt
        ADIE = 0;
        unsigned long tempValue = adcValue;
        ADIE = 1;

        // Calculate value to be displayed
        disp = (90 * tempValue) / 1024;
        
        // Display results to LCD
        ones = disp / 10;           // BCD format
        deci = disp % 10;

        // Display individually
        instCtrl(LINE2);
        dataCtrl(ones + '0');
        dataCtrl('.');
        dataCtrl(deci + '0');
        __delay_ms(100);

        // Send to MCU2
        if(sendSignal){                     // When button is pressed
            while(!TXIF);                   // Wait until TXREG is empty
            TXREG = (deci << 4) | ones;     // Then send BCD data

            sendSignal = 0;                 // Finished transmission
        }
    }   
}

void interrupt ISR(){
    if(ADIE && ADIF){               // When A/D conversion is done
        ADIF = 0;

        // Read results
        adcValue = (ADRESH << 8) | ADRESL;
    }

    else if (INTE && INTF){         // When button is pressed
        INTF = 0;

        if(!sendSignal)             // Drop if still sending
            sendSignal = 1;         // Otherwise, start sending
    }
}

void inline initLCD() {
    __delay_ms(15);     // ~15ms start-up time
    
    instCtrl(0x38);     // 0011 1000 0x38
                        // 001? ??xx Function set
                        // ---1 ---- 8-bit data transfer
                        // ---- 1--- Dual line display
                        // ---- -0-- 5x7 font size
    
    instCtrl(0x08);     // 0000 1000 0x08
                        // 0000 1??? Display ON/OFF
                        // ---- -0-- Entire display off
                        // ---- --0- Cursor off
                        // ---- ---0 Cursor blinking off
    
    instCtrl(0x01);     // 0000 0001 0x01
                        // 0000 0001 Clear display
    
    instCtrl(0x06);     // 0000 0110 0x06
                        // 0000 01?? Entry mode set
                        // ---- --1- Increment / Move right
                        // ---- ---0 No shifting

    instCtrl(0x0E);     // 0000 1110 0x0E
                        // 0000 1??? Display ON/OFF
                        // ---- -1-- Entire display on
                        // ---- --1- Cursor on
                        // ---- ---0 Cursor blinking off 
}

void inline instCtrl(unsigned char inst) {
    PORTD = inst;       //          Send to LCD
    PORTBbits.RB3 = 0;  // RS = 0   Instruction byte
    PORTBbits.RB2 = 0;
    PORTBbits.RB1 = 1;  // E = 1    Enable
    __delay_ms(15);     //          Delay
    PORTBbits.RB1 = 0;  // E = 0    Disable
}

void inline dataCtrl(unsigned char data) {
    PORTD = data;       //          Send to LCD
    PORTBbits.RB3 = 1;  // RS = 1   Data byte
    PORTBbits.RB2 = 0;
    PORTBbits.RB1 = 1;  // E = 1    Enable
    __delay_ms(15);     //          Delay
    PORTBbits.RB1 = 0;  // E = 0    Disable
}

// Displays a string of text to the LCD (no wrap-around logic)
void inline dataStr(const char* string) {
    for(int i = 0; string[i] != '\0'; i++)
        dataCtrl(string[i]);
}