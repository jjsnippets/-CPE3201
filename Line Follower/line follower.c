// Line follower robot
// v2
// 2026/05/12 at 12:20 PM
// Finalize comment block for submission...

#include <xc.h>
#include <pic16f877a.h>
#include <stdint.h>

#pragma config FOSC  = HS       // High-Speed crystal oscillator
#pragma config WDTE  = OFF      // Watchdog Timer disabled
#pragma config PWRTE = ON       // Power-up Timer enabled (stabilises supply)
#pragma config BOREN = ON       // Brown-out Reset enabled
#pragma config LVP   = OFF      // Low-Voltage Programming OFF (frees RB3)
#pragma config CP    = OFF      // Flash code protection off
#pragma config CPD   = OFF      // Data EEPROM code protection off
#pragma config WRT   = OFF      // Flash write protection off

// Hardware configutation
#define _XTAL_FREQ      20000000UL  // 20 MHz

// PID parameters
#define BASE_SPEED_MIN  60          // Minimum base speed (trimmer at 0)
#define BASE_SPEED_MAX  200         // Maximum base speed (trimmer at max)
#define KP              30          // Proportional gain (integer scaled)
#define KI              1           // Integral gain
#define KD              20          // Derivative gain
#define INTEGRAL_MAX    100         // Integral windup clamp
#define INTEGRAL_MIN    (-100)

// Startup delay before robot begins moving (ms)
#define START_DELAY  3000

// Sensors
#define LINE_THRESHOLD 512          // Sensor sees LINE (black) when ADC < LINE_THRESHOLD
#define LOST_LINE_TIME 600          // Broken line grace period (cycles), instead of interrupt logic

// Enable LED outputs
#define LED_OUTPUT 1

// TB6612FNG
#define PWM_DUTY_CAP    138         // To safely drive the motors
                                    // Motors fed directly from 11.1V battery
                                    // cap duty cycle to ~54% (6V/11.1V)

// Port and pin names
#define LED_PORT    PORTB           // LED indicators
#define AIN1    PORTDbits.RD0       // Left  motor dir bit 1
#define AIN2    PORTDbits.RD1       // Left  motor dir bit 2
#define BIN1    PORTDbits.RD2       // Right motor dir bit 1
#define BIN2    PORTDbits.RD3       // Right motor dir bit 2
#define STBY    PORTDbits.RD4       // Driver standby (HIGH = enabled)
#define TRIMMER_CH  6               // AN6 (RE1)

// Convenience macros for movement
#define MOTOR_FORWARD(p1, p2)   do { p1 = 1; p2 = 0; } while(0)
#define MOTOR_REVERSE(p1, p2)   do { p1 = 0; p2 = 1; } while(0)
#define MOTOR_BRAKE(p1, p2)     do { p1 = 1; p2 = 1; } while(0)
#define MOTOR_COAST(p1, p2)     do { p1 = 0; p2 = 0; } while(0)

// Clamp function
#define CLAMP(val, lo, hi)  ((val) < (lo) ? (lo) : ((val) > (hi) ? (hi) : (val)))

// Sensor position weights for weighted-average error calculation
// 0 = Far-left sensor, 5 = Far right sensor
// Scaled by 10 to preserve one decimal place during PID integer arithmetic
static const int16_t SENSOR_WEIGHT[6] = {-50, -30, -10, 10, 30, 50};

// Motor selection
typedef enum {
    MOTOR_LEFT  = 0,
    MOTOR_RIGHT = 1
} MotorID;

// Motor direction states
typedef enum {
    DIR_FORWARD = 0,
    DIR_REVERSE = 1,
    DIR_BRAKE   = 2,
    DIR_COAST   = 3
} MotorDir;

// Robot operational states
typedef enum {
    STATE_IDLE      = 0,    // Stop the Robot (Reserved/unused)
    STATE_FOLLOWING = 1,    // Actively following line
    STATE_LOST      = 2     // Line lost - recovery behaviour active
} RobotState;

// Function prototypes
void SetMotorDirection(MotorID motor, MotorDir dir);
void SetMotorSpeed(MotorID motor, uint8_t speed);
void MotorsSet(uint8_t left_speed, uint8_t right_speed);
void MotorsStop(void);
uint16_t ADCRead(uint8_t channel);
int16_t PIDError(uint8_t sensor_mask);
void inline DelayMS(uint16_t ms);

// Global variables
int16_t  g_last_error = 0;
int16_t  g_integral   = 0;
uint16_t g_lost_counter = 0;       // Time (cycles) since line was last seen

void main() {
    // Local variables
    RobotState state = STATE_FOLLOWING;     // State machine, starting position assumes robot is at line
    uint16_t   sensor_adc[6];               // Raw sensor reading values
    uint8_t    sensor_mask = 0;             // Bitmask of sensor readings (1 == line detected)

    // GPIO ports setup
    // Unused ports set as input
    TRISA = 0x3F; PORTA = 0x00;     // 00ix iiii (0x3F)
                                    // --q- qqqq QTR-8A sensor analog inputs (lhs)
                                    // ---x ---- (RA4 cannot be used as analog input)

    TRISB = 0xC0; PORTB = 0x00;     // iioo oooo (0xC0)
                                    // --ll llll LED line indicator outputs
                                    // pp-- ---- Reserved for ICSP

    TRISC = 0xF9; PORTC = 0x00;     // xxxx xoox (0xF9)
                                    // ---- --b- PWMB output
                                    // ---- -a-- PWMA output

    TRISD = 0xE0; PORTD = 0x00;     // xxxo oooo (0xE0)
                                    // ---- ---a AIN1 output
                                    // ---- --a- AIN2 output
                                    // ---- -b-- BIN1 output
                                    // ---- b--- BIN2 output
                                    // ---s ---- STBY output

    TRISE = 0x07; PORTE = 0x00;     // 0000 0xii (0x07)
                                    // ---- ---q QTR-8A sensor analog inputs (rhs)
                                    // ---- --t- Trimmer analog input

    // A/D Module set-up
    ADCON1 = 0x80;                  // 1000 0000 (0x80)
                                    // ---- 0000 All analog pins
                                    // -0-- ---- FOSC/32 conversion clock (ADCS2)
                                    // 1--- ---- Right justified
    
    ADCON0 = 0x81;                  // 1000 0001 (0x81)
                                    // ---- ---1 Turn on A/D module
                                    // ---- -g-- Clear GO/DONE
                                    // --aa a--- Clear channel select
                                    // 10-- ---- FOSC/32 conversion clock (ADCS1:ADCS0)

    // Timer2 as PWM
    PR2 = 0xFF;                     // (Period length is mostly just for motor smoothness)
                                    // PWM frequency ~4.88 kHz at 20 MHz (FOSC/32, prescale 1:4)
                                    // Duty cycle ceiling = 4*(PR2+1) = 1024 counts.
    T2CON = 0x05;                   // 0000 0101 (0x05)
                                    // ---- --01 1:4 prescaler (PWM period = (255+1)*4*4*(1/20MHz) ~= 204.8us)
                                    // ---- -1-- Turn on Timer2
                                    // -000 ---- 1:1 postscaler (unused for PWM, only affects TMR2IF rate)
    
    // CCP modules setup (both as PWM)
    CCP1CON = 0x0C;                 // 0000 1100 (0x0C)
                                    // ---- 11xx CCP1 as PWM
    CCPR1L = 0x00;                  // --00 ---- initialize to 0% duty cycle

    CCP2CON = 0x0C;                 // 0000 1100 (0x0C)
                                    // ---- 11xx CCP2 as PWM
    CCPR2L = 0x00;                  // --00 ---- initialize to 0% duty cycle

    // Start motors stopped
    STBY = 1;
    MotorsStop();
    
    // Read trimmer at start for base speed
    uint16_t adc_val  = ADCRead(TRIMMER_CH);
    //  base_speed as value between BASE_SPEED_MIN to BASE_SPEED_MAX (linear)
    uint8_t  base_speed = (uint8_t)(BASE_SPEED_MIN +
        (uint8_t)((uint32_t)adc_val * (BASE_SPEED_MAX - BASE_SPEED_MIN)/ 1023UL));

    // Startup delay
    DelayMS(START_DELAY);

    // Give robot initial kick at start, assuming track is initially straight
    SetMotorDirection(MOTOR_LEFT, DIR_FORWARD);
    SetMotorDirection(MOTOR_RIGHT, DIR_FORWARD);
    MotorsSet(base_speed, base_speed);

    // Main control loop
    while(1) {
        // Read all sensors
        for(uint8_t i = 0; i < 6; i++)
            sensor_adc[i] = ADCRead(i);
        
        // Convert ADC readings to a bitmask
        // Bit i is SET when sensor i sees the line
        sensor_mask = 0;
        for(uint8_t i = 0; i < 6; i++)
            if(sensor_adc[i] < LINE_THRESHOLD)
                sensor_mask |= (uint8_t)(1U << i);

        // Update LED displays
        #if LED_OUTPUT == 1
            LED_PORT = (sensor_mask & 0b00111111);
        #endif

        // State machine
        switch(state) {
            // When robot is on line
            case STATE_FOLLOWING: {
                // If robot loses line...
                if(sensor_mask == 0){
                    g_lost_counter++;

                    // If time period is longer than grace period for broken lines
                    if(g_lost_counter >= LOST_LINE_TIME) {
                        state = STATE_LOST;
                        g_lost_counter = 0;
                    }

                    // Otherwise coast forward
                    // Keeping PID and motor values the same
                    break;
                }

                // Otherwise line is found
                g_lost_counter = 0;

                // PID integer computation
                int16_t error = PIDError(sensor_mask);
                int16_t derivative = error - g_last_error;
                g_integral += error;
                g_integral = (int16_t) CLAMP(g_integral, INTEGRAL_MIN, INTEGRAL_MAX);

                int16_t pid = (int16_t)(
                    ((int32_t)KP * error) / 10 +
                    ((int32_t)KI * g_integral) / 10 +
                    ((int32_t)KD * derivative) / 10
                );

                g_last_error = error;

                // Set individual motor speeds
                int16_t left_spd  = (int16_t) base_speed - pid;
                        left_spd  = (int16_t) CLAMP(left_spd,  0, PWM_DUTY_CAP);
                int16_t right_spd = (int16_t) base_speed + pid;
                        right_spd = (int16_t) CLAMP(right_spd, 0, PWM_DUTY_CAP);

                MotorsSet((uint8_t)left_spd, (uint8_t)right_spd);
                break;
            }

            // When robot is not on line
            case STATE_LOST: {
                // If line is found again
                if(sensor_mask != 0) {
                    state = STATE_FOLLOWING;
                    g_integral = 0;     // Reset integral on re-acquire
                    break;
                }

                // Otherwise, robot is still lost
                // Reduce speed to avoid overshooting the line
                uint8_t recovery_speed = (uint8_t)(base_speed / 2);

                if(g_last_error < 0) {  // If line was just to the left, then spin left
                    SetMotorSpeed(MOTOR_LEFT,  0);
                    SetMotorSpeed(MOTOR_RIGHT, recovery_speed);
                                        // Otherwise, line was just to the right and then spin right
                } else {                // If g_last_error == 0 (i.e. robot was perfectly centered when it lost the line), then also spin right
                    SetMotorSpeed(MOTOR_LEFT,  recovery_speed);
                    SetMotorSpeed(MOTOR_RIGHT, 0);
                }

                break;
            }

            // Non-running states
            case STATE_IDLE:
            default:
                MotorsStop();
                break;
        }
    }
}

// Set direction of an individual motor (independent of speed)
void SetMotorDirection(MotorID motor, MotorDir dir) {
    if (motor == MOTOR_LEFT){
        switch(dir) {
            case DIR_FORWARD: MOTOR_FORWARD(AIN1, AIN2); break;
            case DIR_REVERSE: MOTOR_REVERSE(AIN1, AIN2); break;
            case DIR_BRAKE:   MOTOR_BRAKE(AIN1, AIN2);   break;
            case DIR_COAST:
            default:          MOTOR_COAST(AIN1, AIN2);   break;
        }
    } else {
        switch(dir) {
            case DIR_FORWARD: MOTOR_FORWARD(BIN1, BIN2); break;
            case DIR_REVERSE: MOTOR_REVERSE(BIN1, BIN2); break;
            case DIR_BRAKE:   MOTOR_BRAKE(BIN1, BIN2);   break;
            case DIR_COAST:
            default:          MOTOR_COAST(BIN1, BIN2);   break;
        }
    }
}

// Set speed of an individual motor (independent of direction)
void SetMotorSpeed(MotorID motor, uint8_t speed) {
    uint8_t clamp = (speed > PWM_DUTY_CAP) ? (uint8_t)PWM_DUTY_CAP : speed;

    if (motor == MOTOR_LEFT)    CCPR1L = clamp;
    else                        CCPR2L = clamp;
}

// Set both motor speeds simultaneously (directions unchanged)
void MotorsSet(uint8_t left_speed, uint8_t right_speed) {
    SetMotorSpeed(MOTOR_LEFT,  left_speed);
    SetMotorSpeed(MOTOR_RIGHT, right_speed);
}

// Brake both motors
void MotorsStop(void) {
    SetMotorDirection(MOTOR_LEFT,  DIR_BRAKE);
    SetMotorDirection(MOTOR_RIGHT, DIR_BRAKE);
    SetMotorSpeed(MOTOR_LEFT,  0);
    SetMotorSpeed(MOTOR_RIGHT, 0);
}

// Reads a single analog channel (0-7)
// Returns 10-bit ADC result (0-1023), right-justified
uint16_t ADCRead(uint8_t channel) {
    // Select channel
    ADCON0 = (ADCON0 & 0b11000001) | ((uint8_t)(channel << 3) & 0b00111000);

    __delay_us(20);                 // Acquisition time

    ADCON0bits.GO_DONE = 1;         // Start conversion
    while(ADCON0bits.GO_DONE);      // Wait for conversion to finish

    // Right-justified result
    return ((uint16_t)(ADRESH << 8) | ADRESL);
}

// Compute weighted-average position error from sensor ADC readings
// Negative = line is left, Positive = line is right
int16_t PIDError(uint8_t sensor_mask) {
    int32_t weighted_sum = 0;
    uint8_t count = 0;          // Number of active sensors
    uint8_t i;

    // Accumulate weighted position of each sensor that detects the line
    for(i = 0; i < 6; i++) {
        if(sensor_mask & (1U << i)) {
            weighted_sum += (int32_t)SENSOR_WEIGHT[i];
            count++;
        }
    }

    // Hold last known error when line is lost
    if(count == 0)
        return g_last_error;    

    // Return weighted average
    return (int16_t)(weighted_sum / (int32_t)count);
}

// Software delay supporting values > 255
void inline DelayMS(uint16_t ms) {
    while(ms--)
        __delay_ms(1);
}