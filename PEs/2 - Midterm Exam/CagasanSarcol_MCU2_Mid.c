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

void display(){
    while(!RCIF);
    PORTB = RCREG;
      
}
 
void main() {
    SPBRG = 0x19;
    TXSTA = 0x00;
    RCSTA = 0x00;
    
    //TXSTA configuration
    SYNC = 0;
    BRGH = 1;
    
    //RCSTA configuration
    SPEN = 1;
    RX9 = 0;
    CREN = 1;
    
    //port configuration
    TRISB = 0x00;
    PORTB = 0x00;
    
    while(1) {
        display();
    }
}
