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

// Function prototypes
void inline init_I2C_SLAVE(unsigned char slave_add);
void interrupt ISR();

void main() {
    // GPIO ports set-up
    TRISB = 0x00; PORTB = 0x00; // Output: LED bar graph
    TRISD = 0xFF;               // Input: dip switches

    // I2C peripheral set-up
    init_I2C_SLAVE(0x10);       // Slave device with address 0x10

    // Interrupt set-up
    PEIE = 1; GIE = 1;          // Enable interrupts

    // Main program loop
    while(1);                   // Wait for master
}

void inline init_I2C_SLAVE(unsigned char slave_add) {
    TRISC3 = 1; TRISC4 = 1;     // SCL/SDA as input
    SSPEN = 1;                  // Enable serial port
    SSPM3 = 0; SSPM2 = 1;
        SSPM1 = 1; SSPM0 = 0;   // Slave mode, 7-bit address
    CKP = 1;                    // Release clock
    RCEN = 0; PEN = 0; RSEN = 0;// Clear status bits
    SEN = 0;                    // Clock stretching enabled
    SMP = 1;                    // Slew rate disabled 
    SSPADD = slave_add;         // 7-bit slave address (1 = read, 0 = write)

    SSPIE = 1;  SSPIF = 0;      // Enable SSP interrupts
}

void interrupt ISR() {
    unsigned char temp;

    if(SSPIE && SSPIF){         // I2C interrupt
        SSPIF = 0;
        CKP = 0;                    // Stretch (Hold) clock 

        if(WCOL || SSPOV) {         // If data collision or overflow
            temp = SSPBUF;          // Clear buffer
            WCOL = 0; SSPOV = 0;    // Clear flags
            CKP = 1;                // Release clock
        }

        if(!SSPSTATbits.D_nA && !SSPSTATbits.R_nW) {        // If write to slave
            temp = SSPBUF;          // Clear buffer
            while(!BF);             // Wait until receive is complete
            PORTB = SSPBUF;         // Receive data from master
            CKP = 1;                // Release clock

        }  else if(!SSPSTATbits.D_nA && SSPSTATbits.R_nW) {  // If read from slave
            temp = SSPBUF;          // Clear buffer
            SSPBUF = PORTD;         // Send data to master
            CKP = 1;                // Release clock
            while(BF);              // Wait until transmit is complete
        } 
    }
}