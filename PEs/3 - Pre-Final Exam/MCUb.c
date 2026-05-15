
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

void initLCD();
void instCtrl(unsigned char INST);
void dataCtrl(unsigned char DATA);
void inline dataStr(const char* string);

typedef enum LINES {
    LINE1 = 0x80,
    LINE2 = 0xC0,
    LINE3 = 0x94,
    LINE4 = 0xD4
} LINES;

void main(){
    unsigned char receiveData = 0;

    // GPIO set-up
    TRISB = 0x00;               // PORTB as output
    TRISD = 0x00;               // PORTD as output

    // USART Asynchronous Reception
    SPEN = 1;                   // Serial port enable
    SYNC = 0;                   // Asynchronous mode
    BRGH = 1;                   // High speed
    SPBRG = 0x19;               // 9.6Kbaud at 4MH; 0.16% error
    RX9 = 0;                    // 8-bit reception

    initLCD();
    instCtrl(LINE1);                // "Voltage: "
    dataStr("Voltage: x.x ");
    instCtrl(LINE2);                // "Frequency: "
    dataStr("Frequency: x ");
    instCtrl(LINE3);                // "Count: "
    dataStr("Count: xx ");

    // Foreground routine
    CREN = 1;                   // Start of continuous reception
    while(1) {
        while(!RCIF);           // When RXREG is full
        receiveData = RCREG;    // Then accept data

        unsigned char ones, deci;

        // Display Voltage reading
        ones = receiveData / 10;           // BCD format
        deci = receiveData % 10;

        instCtrl(LINE1 + 9);
        dataCtrl(ones + '0');
        dataCtrl('.');
        dataCtrl(deci + '0');

        // Display Frequency reading
        unsigned char freqDisp = ones;
        if(deci) freqDisp++;

        instCtrl(LINE2 + 11);
        dataCtrl(freqDisp + '0');

        


    }
}

// Helper function to initialize the LCD
void initLCD(){
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

// Helper function to send an instruction byte to LCD
void instCtrl(unsigned char INST){
    PORTB = INST;       //          Send to LCD
    PORTDbits.RD0 = 0;  // RS = 0   Instruction byte
    PORTDbits.RD1 = 1;  // E = 1    Enable
    __delay_ms(15);     //          Delay
    PORTDbits.RD1 = 0;  // E = 0    Disable
}

// Helper function to send a data byte to LCD
void dataCtrl(unsigned char DATA){
    PORTB = DATA;       //          Send to LCD
    PORTDbits.RD0 = 1;  // RS = 1   Data byte
    PORTDbits.RD1 = 1;  // E = 1    Enable
    __delay_ms(15);     //          Delay
    PORTDbits.RD1 = 0;  // E = 0    Disable
}

// Displays a string of text to the LCD (no wrap-around logic)
void inline dataStr(const char* string) {
    for(int i = 0; string[i] != '\0'; i++)
        dataCtrl(string[i]);
}