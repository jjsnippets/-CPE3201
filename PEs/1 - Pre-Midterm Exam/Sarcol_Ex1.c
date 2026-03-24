// CPE 3201 - EMBEDDED SYSTEMS
// Group 4      TTh 7:30 AM - 10:30 AM LB 285 TC
// Sarcol, Joshua S.        BS CpE - 3      2026/03/19
// Pre-Midterm Examination: Practical Exam (CO2)

// Create a code that will function according to the circuit functionality shown by the instructor.
// You may download the Proteus project file here.
//
// Kindly take note of the following specifications:
// - Delay must use Timer 0 Interrupt implementation.
// - Timer Prescaler should be 1:32.
// - Time between counts should be approximately 1 second.
// - Oscillator frequency is 4MHz.
// - Use RB0/INT External Interrupt on button presses to implement the various input characteristics.
// - Use * to decrement, # to increment, 0 to start/stop counts and 8 to reset the timer (10) during a stop state.
// - Count goes from 00 to 10 during count down or decrement operation.
// - Count goes from 10 to 00 during increment operation.
// - Submit your C file using the filename format: FamilyName_Ex1.c

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
void initLCD();
void instCtrl(unsigned char INST);
void dataCtrl(unsigned char DATA);
void delayLoop(unsigned int time);
void delay(unsigned int cnt);

// Keypad array
// 0 = invalid/no keypress
// (*) 1 = increment
// (#) 2 = decrement
// (0) 3 = start/stop
// (8) 4 = set/reset
static unsigned char KEYPAD[] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x04, 0x00, 0x00,
    0x01, 0x03, 0x02, 0x00,
};

// LCD DDRAM Address Locations
typedef enum LINES {
    LINE1 = 0x80,
    LINE2 = 0xC0,
    LINE3 = 0x94,
    LINE4 = 0xD4
} LINES;

// Global variables
unsigned int counter = 10;          // Count that is saved
unsigned int countDisplay = 10;     // Count that is displayed to the LCD
unsigned char input = 0;            // Accepts keypad input
bit timeRunning = 0;                // 0 = stopped, 1 = running
bit timerOverflow = 0;              // Indicates TMR0 overflow

void main(){
    // GPIO ports setup
    TRISB = 0x01;                   // RB0 as input (as interrupt)
    TRISC = 0x00;                   // PORTC as output (LCD DATA)
    TRISD = 0x0F;                   // Lower nibble of PORTD as input (keypad)
                                    // Upper nibble of PORTD as output (LCD INSTRUCTIONS)
                                    //    RD5 = RS
                                    //    RD6 = RW
                                    //    RD7 = E
   
    // Interrupt set-up
    // RB0 as interrupt
    INTEDG = 1;                         // At rising edge
    INTF = 0;                           // Clear flag
    INTE = 1;                           // Enable RB0 interrupt

    // TMR0
    PS2 = 1; PS1 = 0; PS0 = 0;          // 1:32 Prescaler
    PSA = 0;                            // Presacler to TMR0
    T0CS = 0;                           // Internal instruction clock cycle
    T0IF = 0;                           // Clear flag
    T0IE = 1;                           // Enable TMR0 interrupt

    // Initialization
    initLCD();
    instCtrl(LINE2);                    // To start of second line
    dataCtrl('T');                      // Time:
    dataCtrl('i');
    dataCtrl('m');
    dataCtrl('e');
    dataCtrl(':');

    // Enable all interrupts
    GIE = 1; 

    while(1){
        // If timer is running
        if(timeRunning){
        // Then decrement dispplay every second
        countDisplay = (countDisplay == 0) ? 10 : countDisplay--;
        delay(1);
        }
       
        // If keypad is pressed...
        if(input) {
            // If timer is not running
            if(!timeRunning){
                switch(input){
                    case 1:    // Increment
                        counter = (counter == 10) ? 0 : counter++;
                        break;
                    case 2:    // Decrement
                        counter = (counter == 0) ? 10 : counter--;
                        break;
                    case 3:    // Start
                        timeRunning = 1;
                        break;
                    case 4:    // Reset to 10
                        counter = 10;
                        break;
                }
                // Update display
                countDisplay = counter;
       
            // If timer is running
            } else {
                switch(input){
                    case 3:        // Stop
                        timeRunning = 0;
                        counter = countDisplay;
                        break;
                    case 4:        // Set
                        countDisplay = counter; 
                }
            }
        }
       
        // Eat up input, if any
        input = 0;            
       
        // Display time to LCD
        unsigned int tens = countDisplay / 10;
        unsigned int ones = countDisplay % 10;
       
        instCtrl(LINE3);                // To start of third line
        dataCtrl(tens + '0');           // 10 s
        dataCtrl(ones + '0');
        dataCtrl(' ');
        dataCtrl('s');
    }
}

void interrupt ISR(){
    GIE = 0;

    if(INTF) {                          // If RB0 is triggered
        INTF = 0;                       // Clear RB0 flag

        input = KEYPAD[PORTD & 0x0F];   // Accept keypad input

    } else
    if(T0IF) {                          // If TMR0 is overflowed
        T0IF = 0;

        timerOverflow = 1;              // Indicate TMR0 overflow for delay()
    }

    GIE = 1;
}

// Helper function to initialize the LCD
void initLCD(){
    delayLoop(15);      // ~15ms start-up time
    
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
    PORTC = INST;       //          Send to LCD
    PORTDbits.RD5 = 0;  // RS = 0   Instruction byte
    PORTDbits.RD6 = 0;
    PORTDbits.RD7 = 1;  // E = 1    Enable
    delayLoop(15);      //          Delay
    PORTDbits.RD7 = 0;  // E = 0    Disable
}

// Helper function to send a data byte to LCD
void dataCtrl(unsigned char DATA){
    PORTC = DATA;       //          Send to LCD
    PORTDbits.RD5 = 1;  // RS = 1   Data byte
    PORTDbits.RD6 = 0;
    PORTDbits.RD7 = 1;  // E = 1    Enable
    delayLoop(15);      //          Delay
    PORTDbits.RD7 = 0;  // E = 0    Disable
}

// Software-defined delay
void delayLoop(unsigned int time){
    for (int i = 0; i < time; i++)
        for (int j = 0; j < 125; j++);
}

// Hardware-defined delay
void delay(unsigned int cnt){
    unsigned int u_cnt = cnt * 122;     // 1 cnt approx 1 second
    for (int i = 0; i < u_cnt; i++){
        while(!timerOverflow);          // Wait until TMR0 overflows
        timerOverflow = 0;              // Wait again u_cnt times
    }
}

