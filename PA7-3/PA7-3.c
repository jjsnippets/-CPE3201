// CPE 3201 - EMBEDDED SYSTEMS
// Group 4      TTh 7:30 AM - 10:30 AM LB 285 TC
// Sarcol, Joshua S.        BS CpE - 3      2026/05/04
// Practical Activity 7: Inter-Integrated Circuit Communication (I2C)

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

// Convenience macros
#define ACK 1
#define NACK 0

// LCD DDRAM Address Locations
typedef enum LINES {
    LINE1 = 0x80,
    LINE2 = 0xC0,
    LINE3 = 0x94,
    LINE4 = 0xD4
} LINES;

// Function prototypes
void inline init_I2C_MASTER();
void inline I2C_send(unsigned char data);
unsigned char inline I2C_receive(unsigned char ack);
void inline I2C_wait();
void inline I2C_start();
void inline I2C_stop();
void inline I2C_repeatedStart();
void inline initLCD();
void inline instCtrl(unsigned char inst);
void inline dataCtrl(unsigned char data);
void inline dataStr(const char* string);
void inline valueDisp(unsigned char value);

void main() {
    unsigned short int rhSensor, tempSensor;
    unsigned char rhData;
    signed short int tempData;
    unsigned char rh2, rh1, rh0, t2, t1, t0;
    unsigned char temp;

    // GPIO ports set-up
    TRISB = 0x00; PORTB = 0x00;     // Output: LCD data lines
    TRISD = 0x00;                   // Output: LCD control lines

    // I2C peripheral set-up
    init_I2C_MASTER();              // Master device

    // LCD initialization
    initLCD();
    instCtrl(LINE1);                // "Humidity: "
    dataStr("Humidity: ");
    instCtrl(LINE3);                // "Temperature: "
    dataStr("Temperature: ");

    // Main program loop
    while(1){
        // Humidity Reading
        I2C_repeatedStart();            // Repeated communication
        I2C_send(0x80);                 // SHT21 address (write)
        I2C_send(0xE5);                 // RH reading (with clock stretching)
        I2C_repeatedStart();            // Change direction
        I2C_send(0x81);                 // SHT21 address (read)
        temp = I2C_receive(ACK);    
            rhSensor = (temp << 8);     // MSB of RH
        temp = I2C_receive(NACK);
            rhSensor |= temp;           // LSB of RH
            rhSensor &= 0xFFFC;         // Remove status bits
        I2C_stop();

        // Display Humidity
        rhData = (unsigned char)(-6.0 + 125.0 *((float) rhSensor / 65536.0));
        
        instCtrl(LINE2);        // Move to 4th line
        dataStr("         ");
            instCtrl(LINE2);    // Clear previously displayed value
        valueDisp(rhData);      // Display digits
        dataCtrl('%');          // Display units
        
        // Temperature reading
        I2C_repeatedStart();            // Repeated communication
        I2C_send(0x80);                 // SHT21 address (write)
        I2C_send(0xE3);                 // Temp reading (with clock stretching)
        I2C_repeatedStart();            // Change direction
        I2C_send(0x81);                 // SHT21 address (read)
        temp = I2C_receive(ACK);    
            tempSensor = (temp << 8);   // MSB of Temp
        temp = I2C_receive(NACK);
            tempSensor |= temp;         // LSB of Temp
            tempSensor &= 0xFFFC;       // Remove status bits
        I2C_stop();
        
        // Display Temperature
        float tempFloat = -46.85 + 175.72 * ((float)tempSensor / 65536.0);
        if (tempFloat >= 0)     // Round to the nearest integer
            tempData = (signed short int)(tempFloat + 0.5);
        else
            tempData = (signed short int)(tempFloat - 0.5);
        
        instCtrl(LINE4);        // Move to 4th line
        dataStr("         ");
            instCtrl(LINE4);    // Clear previously displayed value

        // Display negative sign if necessary
        if(tempData < 0){
            dataCtrl('-');      // Show negative sign
            tempData *= -1;     // Chnage internal value to positive
        }

        valueDisp(tempData);
        dataStr(" C");
    }   
}

void inline init_I2C_MASTER() {
    TRISC3 = 1; TRISC4 = 1;     // SCL/SDA as input
    SSPEN = 1;                  // Enable serial port
    SSPM3 = 1; SSPM2 = 0;
        SSPM1 = 0; SSPM0 = 0;   // Master mode
    RCEN = 0; PEN = 0;
        RSEN = 0; SEN = 0;      // Clear status bits
    SMP = 0;                    // Slew rate enabled (400 kHz) 
    SSPADD = 0x09;              // 100KHz clock speed at Fosc = 4MHz
}

void inline I2C_send(unsigned char data) {
    I2C_wait();
    SSPBUF = data;              // Write data to buffer
}

unsigned char inline I2C_receive(unsigned char ack){
    unsigned char temp;

    I2C_wait(); RCEN = 1;       // Start of reception
    I2C_wait(); temp = SSPBUF;  // Read buffer

    I2C_wait();
    ACKDT = ack ? 0 : 1;        // 0 = ACK (send next byte), 1 = NACK (release clock)
    ACKEN = 1;                  // Initiate acknowledge sequence

    return temp;
}

void inline I2C_wait() {
    while((SSPCON2 & 0x1F) || (SSPSTAT & 0x04));
}

void inline I2C_start() {
    I2C_wait(); SEN = 1;        // Initiate start condition
}

void inline I2C_stop() {
    I2C_wait(); PEN = 1;        // Initiate stop condition
}

void inline I2C_repeatedStart() {
    I2C_wait(); RSEN = 1;       // Initiate repeated start condition
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
    PORTB = inst;       //          Send to LCD
    PORTDbits.RD0 = 0;  // RS = 0   Instruction byte
    PORTDbits.RD1 = 1;  // E = 1    Enable
    __delay_ms(15);     //          Delay
    PORTDbits.RD1 = 0;  // E = 0    Disable
}

void inline dataCtrl(unsigned char data) {
    PORTB = data;       //          Send to LCD
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

// Helper function to display a 3-digit number (positive only)
// Assumes that location in LCD is already set-up
void inline valueDisp(unsigned char value) {
    unsigned char p2, p1, p0;

    // Extracting individual digits
    p2 = value / 100;
    p1 = (value % 100) / 10;
    p0 = value % 10;

    // Display number
    if(p2)                      // If hundreds place is 0
        dataCtrl(p2 + '0');     // Then do not display hundreds place

    if(p2 || p1)                // And if tens place is 0
        dataCtrl(p1 + '0');     // Then do not also display tens place

    dataCtrl(p0 + '0');         // Display ones place no matter what

}