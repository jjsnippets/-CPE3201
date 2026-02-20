#include <xc.h>

#pragma config FOSC = XT
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

void main() {
    int cnt;
    TRISA = 0x01;                   // PORTA as input
    TRISB = 0x00;                   // set all bits (port) in PORTB as output
    ADCON1 = 0x06;                  // All\ read to be digital
    
    for(;;){
        if(PORTAbits.RA0 == 1){
            for(int i = 0; i < 3; i++){
                PORTB = 0x01;               // set RB0 to 1 (LED ON)
                for(cnt=0;cnt<10000;cnt++); // delay
                PORTB = 0x00;               // set RB0 to 0 (LED OFF)
                for(cnt=0;cnt<10000;cnt++); // delay
            }
        }
        else
            PORTB = 0x00;
    }
}