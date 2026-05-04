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

// Function prototypes
void inline init_I2C_MASTER();
void inline I2C_send(unsigned char data);
unsigned char inline I2C_receive(unsigned char ack);
void inline I2C_wait();
void inline I2C_start();
void inline I2C_stop();
void inline I2C_repeatedStart();

void main() {
    // GPIO ports set-up
    TRISB = 0x00; PORTB = 0x00; // Output: LED bar graph
    TRISD = 0xFF;               // Input: dip switches

    // I2C peripheral set-up
    init_I2C_MASTER();          // Master device

    // Main program loop
    while(1){
        I2C_start();                // Start of communication
        I2C_send(0x10);             // Slave 0x10 + write
        I2C_send(PORTD);            // Send dip switch data to slave
        I2C_stop();                 // End of transmission
        __delay_ms(200);

        I2C_repeatedStart();        // Start of repeated communication
        I2C_send(0x11);             // Slave 0x10 + read
        PORTB = I2C_receive(NACK);  // Accept data from slave
                                    // NACK to end communication
        I2C_stop();                 // End of reception
        __delay_ms(200);
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