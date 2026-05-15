// =============================================================================
// Line Follower Robot - PIC16F877A
// =============================================================================
// Microcontroller : PIC16F877A @ 20 MHz (HS oscillator)
// Compiler        : MPLAB XC8 v1.33
// IDE             : MPLAB IDE v8.2
//
// Hardware Summary:
//   Sensors  : Pololu QTR-8A (6 of 8 used) -> AN0-AN5 (RA0-RA5, RE0)
//   Trimmer  : Speed potentiometer          -> AN6 (RE1)
//   Motors   : TB6612FNG dual H-bridge
//     PWM    : RC2/CCP1 (Left), RC1/CCP2 (Right)
//     DIR    : RD0/AIN1, RD1/AIN2, RD2/BIN1, RD3/BIN2
//     STBY   : RD4
//   LEDs     : RB0-RB5 (one per sensor channel, active-high)
//   Reset    : MCLR button (hardware)
// =============================================================================

// -----------------------------------------------------------------------------
// Microcontroller files
// -----------------------------------------------------------------------------
#include <xc.h>
#include <pic16f877a.h>
#include <stdint.h>

// -----------------------------------------------------------------------------
// Configuration bits
// -----------------------------------------------------------------------------
// NOTE: FOSC=HS for 20MHz crystal. Change to XT only for <= 4MHz crystals.
#pragma config FOSC  = HS       // High-Speed crystal oscillator
#pragma config WDTE  = OFF      // Watchdog Timer disabled (enable for production)
#pragma config PWRTE = ON       // Power-up Timer enabled (stabilises supply)
#pragma config BOREN = ON       // Brown-out Reset enabled
#pragma config LVP   = OFF      // Low-Voltage Programming OFF (frees RB3)
#pragma config CP    = OFF      // Flash code protection off
#pragma config CPD   = OFF      // Data EEPROM code protection off
#pragma config WRT   = OFF      // Flash write protection off

// -----------------------------------------------------------------------------
// Hardware configuration
// -----------------------------------------------------------------------------
#define _XTAL_FREQ      20000000UL  // 20 MHz - used by __delay_ms() / __delay_us()

// --- Sensor ADC channels (QTR-8A, 6 sensors, AN0-AN5) ---
#define SENSOR_COUNT        6
#define SENSOR_CH_FIRST     0       // AN0 (RA0) = sensor 1, far left
#define SENSOR_CH_LAST      5       // AN5 (RE0) = sensor 6, far right

// --- Trimmer ADC channel ---
#define TRIMMER_CH          6       // AN6 (RE1)

// --- TB6612FNG direction control (Port D) ---
#define AIN1    PORTDbits.RD0       // Left  motor dir bit 1
#define AIN2    PORTDbits.RD1       // Left  motor dir bit 2
#define BIN1    PORTDbits.RD2       // Right motor dir bit 1
#define BIN2    PORTDbits.RD3       // Right motor dir bit 2
#define STBY    PORTDbits.RD4       // Driver standby (HIGH = enabled)

// --- LED indicators (Port B, one per sensor) ---
#define LED_PORT    PORTB           // Write entire nibble at once for speed

// --- ADC timing (acquisition + conversion) ---
// At 20 MHz, FOSC/32 gives TAD = 1.6 us (within 1.6-6.4 us spec for 10-bit)
#define ADC_FOSC_DIV    0b101       // ADCS1:ADCS0 = 10 (FOSC/32) -> set in ADCON0

// --- PWM parameters ---
// Timer2: PR2=0xFF, prescale=4 -> PWM period = (255+1)*4*4*(1/20MHz) = 204.8 us (~4.88 kHz)
#define PWM_PERIOD      0xFF        // PR2 value
#define PWM_MAX         255         // Maximum 8-bit duty cycle value (upper 8 of 10 bits)
#define PWM_MIN         0

// --- Speed limits ---
// If motors fed from 11.1V battery directly (no buck), cap duty to ~54% (6V/11.1V).
// If using LM2596 buck at 6V, set CAP to PWM_MAX.
#define PWM_DUTY_CAP    255         // Change to 138 if no buck converter is used

// --- PID defaults (tuned empirically; trimmer adjusts BASE_SPEED at runtime) ---
#define BASE_SPEED_MIN  60          // Minimum base speed (trimmer at 0)
#define BASE_SPEED_MAX  200         // Maximum base speed (trimmer at max)
#define KP              30          // Proportional gain (integer scaled)
#define KI              1           // Integral gain
#define KD              20          // Derivative gain

// --- Integral windup clamp ---
#define INTEGRAL_MAX    100
#define INTEGRAL_MIN    (-100)

// --- Startup delay before robot begins moving (ms) ---
#define START_DELAY_MS  3000

// --- Sensor thresholds ---
// QTR-8A analog output: higher voltage = more reflection (white surface).
// ADC result is 10-bit (0-1023). A mid-point threshold distinguishes line from background.
// Adjust LINE_THRESHOLD based on actual surface/sensor characterisation.
#define LINE_THRESHOLD  512         // ADC value above which sensor sees white (no line)
                                    // Sensor sees LINE (black) when ADC < LINE_THRESHOLD

// Sensor position weights for weighted-average error calculation.
// Index 0 = far left (most negative), index 5 = far right (most positive).
// Scaled by 10 to preserve one decimal place in integer arithmetic.
static const int16_t SENSOR_WEIGHT[SENSOR_COUNT] = {-50, -30, -10, 10, 30, 50};

// -----------------------------------------------------------------------------
// Convenience macros
// -----------------------------------------------------------------------------
#define CLAMP(val, lo, hi)  ((val) < (lo) ? (lo) : ((val) > (hi) ? (hi) : (val)))

#define MOTOR_FORWARD(ain1, ain2)   do { ain1 = 1; ain2 = 0; } while(0)
#define MOTOR_REVERSE(ain1, ain2)   do { ain1 = 0; ain2 = 1; } while(0)
#define MOTOR_BRAKE(ain1, ain2)     do { ain1 = 1; ain2 = 1; } while(0)
#define MOTOR_COAST(ain1, ain2)     do { ain1 = 0; ain2 = 0; } while(0)

#define DRIVER_ENABLE()     do { STBY = 1; } while(0)
#define DRIVER_DISABLE()    do { STBY = 0; } while(0)

#define LED_SET(mask)       do { LED_PORT = (uint8_t)(mask); } while(0)

// -----------------------------------------------------------------------------
// Enumerations
// -----------------------------------------------------------------------------

// Motor selection (used in SetMotorSpeed / SetMotorDirection)
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
    STATE_IDLE      = 0,    // Waiting before start delay expires
    STATE_FOLLOWING = 1,    // Actively following line
    STATE_LOST      = 2     // Line lost - recovery behaviour active
} RobotState;

// -----------------------------------------------------------------------------
// Function prototypes
// -----------------------------------------------------------------------------

// Initialisation
void Init_Ports(void);
void Init_ADC(void);
void Init_PWM(void);
void Init_Timer2(void);

// ADC
uint16_t ADC_Read(uint8_t channel);
void     Sensors_Read(uint16_t *results);
uint8_t  Sensors_GetBitmask(const uint16_t *results);

// Motor control
void SetMotorSpeed(MotorID motor, uint8_t speed);
void SetMotorDirection(MotorID motor, MotorDir dir);
void Motors_Set(uint8_t left_speed, uint8_t right_speed);
void Motors_Stop(void);

// PID
int16_t  PID_ComputeError(const uint16_t *sensor_results, uint8_t *active_count);
uint8_t  Trimmer_ReadBaseSpeed(void);

// LED
void LED_Update(uint8_t sensor_mask);

// Utility
void Delay_ms(uint16_t ms);

// -----------------------------------------------------------------------------
// Global variables (kept minimal - PIC16F877A has only 368 bytes RAM)
// -----------------------------------------------------------------------------
static volatile uint8_t  g_pwm_tick = 0;   // Reserved for future ISR use

static int16_t  g_last_error = 0;
static int16_t  g_integral   = 0;

// -----------------------------------------------------------------------------
// main()
// -----------------------------------------------------------------------------
void main(void)
{
    // --- Initialise all peripherals ---
    Init_Ports();
    Init_ADC();
    Init_Timer2();
    Init_PWM();

    // --- Enable TB6612FNG ---
    DRIVER_ENABLE();

    // --- Set both motors forward, stopped ---
    SetMotorDirection(MOTOR_LEFT,  DIR_FORWARD);
    SetMotorDirection(MOTOR_RIGHT, DIR_FORWARD);
    Motors_Stop();

    // --- Read trimmer to set base speed ---
    uint8_t base_speed = Trimmer_ReadBaseSpeed();

    // --- Startup delay: place robot on track before it moves ---
    Delay_ms(START_DELAY_MS);

    // --- State machine variables ---
    RobotState state       = STATE_FOLLOWING;
    uint16_t   sensor_adc[SENSOR_COUNT];
    uint8_t    sensor_mask = 0;
    uint8_t    active_count = 0;

    // --- Main control loop ---
    while(1)
    {
        // 1. Read all 6 sensors
        Sensors_Read(sensor_adc);
        sensor_mask  = Sensors_GetBitmask(sensor_adc);
        LED_Update(sensor_mask);

        // 2. Periodically refresh base speed from trimmer
        //    (done every ~50 cycles to avoid slowing the loop)
        static uint8_t trimmer_divider = 0;
        if(++trimmer_divider >= 50)
        {
            trimmer_divider = 0;
            base_speed = Trimmer_ReadBaseSpeed();
        }

        // 3. State machine
        switch(state)
        {
            case STATE_FOLLOWING:
            {
                int16_t error = PID_ComputeError(sensor_adc, &active_count);

                if(active_count == 0)
                {
                    // Line lost - transition to recovery
                    state = STATE_LOST;
                    break;
                }

                // PID computation (integer arithmetic only - no float)
                int16_t derivative = error - g_last_error;
                g_integral        += error;
                g_integral         = (int16_t)CLAMP(g_integral, INTEGRAL_MIN, INTEGRAL_MAX);

                int16_t pid = (int16_t)(
                    ((int32_t)KP * error) / 10 +
                    ((int32_t)KI * g_integral) / 10 +
                    ((int32_t)KD * derivative) / 10
                );

                g_last_error = error;

                // Compute individual motor speeds
                int16_t left_spd  = (int16_t)base_speed - pid;
                int16_t right_spd = (int16_t)base_speed + pid;

                // Clamp to [0, PWM_DUTY_CAP]
                left_spd  = (int16_t)CLAMP(left_spd,  PWM_MIN, PWM_DUTY_CAP);
                right_spd = (int16_t)CLAMP(right_spd, PWM_MIN, PWM_DUTY_CAP);

                Motors_Set((uint8_t)left_spd, (uint8_t)right_spd);
                break;
            }

            case STATE_LOST:
            {
                // Recovery: slow spin in direction of last known error,
                // reduced speed to avoid overshooting the line
                uint8_t recovery_speed = (uint8_t)(base_speed / 2);

                if(g_last_error < 0)
                {
                    // Line was to the left - spin left
                    SetMotorSpeed(MOTOR_LEFT,  0);
                    SetMotorSpeed(MOTOR_RIGHT, recovery_speed);
                }
                else
                {
                    // Line was to the right - spin right
                    SetMotorSpeed(MOTOR_LEFT,  recovery_speed);
                    SetMotorSpeed(MOTOR_RIGHT, 0);
                }

                // Check if line is found again
                if(active_count > 0)
                {
                    g_integral = 0;     // Reset integral on re-acquire
                    state = STATE_FOLLOWING;
                }

                // Re-read sensors within same cycle for faster re-acquisition
                Sensors_Read(sensor_adc);
                sensor_mask  = Sensors_GetBitmask(sensor_adc);
                active_count = __builtin_popcount(sensor_mask);  // Count active bits
                LED_Update(sensor_mask);
                break;
            }

            case STATE_IDLE:
            default:
                Motors_Stop();
                break;
        }
    }
}

// -----------------------------------------------------------------------------
// Interrupt Service Routine
// -----------------------------------------------------------------------------
void interrupt ISR(void)
{
    // Timer2 interrupt is not used in this implementation.
    // Reserved for future use (e.g. encoder feedback, scheduled telemetry).
    // TMR2 drives PWM automatically in hardware - no ISR needed for PWM.

    if(PIR1bits.TMR2IF)
    {
        PIR1bits.TMR2IF = 0;    // Clear flag
        g_pwm_tick++;           // Free-running tick counter (available for future use)
    }
}

// =============================================================================
// Initialisation functions
// =============================================================================

// -----------------------------------------------------------------------------
// Init_Ports()
// Configure TRIS (direction) registers for all I/O pins.
// All unused pins set as inputs to prevent floating drive currents.
// -----------------------------------------------------------------------------
void Init_Ports(void)
{
    // --- Port A: RA0-RA5 as inputs (ADC), RA4 as input (open-drain, unused) ---
    TRISA = 0xFF;           // All Port A pins = input
    PORTA = 0x00;

    // --- Port B: RB0-RB5 outputs (LEDs), RB6-RB7 inputs (ICSP - must stay input) ---
    TRISB = 0b11000000;     // RB6, RB7 input; RB0-RB5 output
    PORTB = 0x00;           // All LEDs off

    // --- Port C: RC1 (CCP2) and RC2 (CCP1) outputs; rest inputs ---
    TRISC = 0b11111001;     // RC1=out, RC2=out; RC0,RC3-RC7=in
    PORTC = 0x00;

    // --- Port D: RD0-RD4 outputs (motor direction + STBY); RD5-RD7 inputs ---
    TRISD = 0b11100000;     // RD0-RD4 output; RD5-RD7 input
    PORTD = 0x00;           // All direction pins low; STBY low (disabled until ready)

    // --- Port E: RE0-RE1 inputs (ADC), RE2 input (unused/pulled to GND) ---
    TRISE = 0b00000111;     // RE0, RE1, RE2 all input
    PORTE = 0x00;

    // --- ADCON1: Configure analog/digital pin assignment ---
    // AN0-AN6 analog (sensors + trimmer), AN7 digital, result right-justified.
    // See PIC16F877A datasheet Table 11-3.
    // ADFM=1 (right justify), PCFG=0001 -> AN0-AN6 analog, AN7 digital, Vref=VDD/VSS
    ADCON1 = 0b10000001;
}

// -----------------------------------------------------------------------------
// Init_ADC()
// Configure the ADC module. Channel selection happens per-read in ADC_Read().
// -----------------------------------------------------------------------------
void Init_ADC(void)
{
    // ADCON0: ADCS=10 (FOSC/32=1.6us TAD @20MHz), CHS=000 (start on AN0), ADON=1
    // Bit layout: ADCS1 ADCS0 CHS2 CHS1 CHS0 GO/DONE - ADON
    ADCON0 = 0b10000001;    // FOSC/32, channel 0, ADC ON
    __delay_us(20);         // Allow ADC to stabilise after power-on
}

// -----------------------------------------------------------------------------
// Init_Timer2()
// Timer2 is the PWM time base for both CCP modules.
// PR2=0xFF, prescale=4 -> ~4.88 kHz PWM frequency at 20 MHz.
// -----------------------------------------------------------------------------
void Init_Timer2(void)
{
    PR2    = PWM_PERIOD;        // Period register
    T2CON  = 0b00000101;        // Prescaler 1:4, Timer2 ON, postscaler 1:1

    PIR1bits.TMR2IF  = 0;       // Clear interrupt flag
    PIE1bits.TMR2IE  = 1;       // Enable TMR2 interrupt (for g_pwm_tick)
    INTCONbits.PEIE  = 1;       // Enable peripheral interrupts
    INTCONbits.GIE   = 1;       // Enable global interrupts
}

// -----------------------------------------------------------------------------
// Init_PWM()
// Configure CCP1 (RC2, left motor) and CCP2 (RC1, right motor) for PWM mode.
// Timer2 must be configured before calling this.
// -----------------------------------------------------------------------------
void Init_PWM(void)
{
    // CCP1 - PWM mode (RC2)
    CCP1CON = 0b00001100;   // CCP1M = 1100 (PWM mode), DC1B1:DC1B0 = 00
    CCPR1L  = 0;            // 0% duty cycle initially

    // CCP2 - PWM mode (RC1)
    CCP2CON = 0b00001100;   // CCP2M = 1100 (PWM mode), DC2B1:DC2B0 = 00
    CCPR2L  = 0;            // 0% duty cycle initially
}

// =============================================================================
// ADC functions
// =============================================================================

// -----------------------------------------------------------------------------
// ADC_Read()
// Read a single ADC channel (0-7). Returns 10-bit result (0-1023).
// Acquisition time: ~20 us minimum at 20 MHz with FOSC/32.
// -----------------------------------------------------------------------------
uint16_t ADC_Read(uint8_t channel)
{
    // Select channel
    ADCON0 = (ADCON0 & 0b11000001) | ((uint8_t)(channel << 3) & 0b00111000);

    __delay_us(20);                 // Acquisition time

    ADCON0bits.GO_DONE = 1;         // Start conversion
    while(ADCON0bits.GO_DONE);      // Wait for completion (~12 TAD = ~19.2 us)

    // Right-justified result (ADFM=1): ADRESH:ADRESL
    return ((uint16_t)(ADRESH << 8) | ADRESL);
}

// -----------------------------------------------------------------------------
// Sensors_Read()
// Read all 6 sensor channels into the provided results array.
// results[] must have at least SENSOR_COUNT elements.
// -----------------------------------------------------------------------------
void Sensors_Read(uint16_t *results)
{
    uint8_t i;
    for(i = 0; i < SENSOR_COUNT; i++)
    {
        results[i] = ADC_Read((uint8_t)(SENSOR_CH_FIRST + i));
    }
}

// -----------------------------------------------------------------------------
// Sensors_GetBitmask()
// Convert ADC readings to a bitmask. Bit i is SET when sensor i sees the LINE.
// LINE condition: ADC value < LINE_THRESHOLD (dark surface absorbs IR -> low output).
// -----------------------------------------------------------------------------
uint8_t Sensors_GetBitmask(const uint16_t *results)
{
    uint8_t mask = 0;
    uint8_t i;
    for(i = 0; i < SENSOR_COUNT; i++)
    {
        if(results[i] < LINE_THRESHOLD)
            mask |= (uint8_t)(1 << i);
    }
    return mask;
}

// =============================================================================
// Motor control functions
// =============================================================================

// -----------------------------------------------------------------------------
// SetMotorSpeed()
// Set PWM duty cycle for one motor. Speed 0-255 (upper 8 bits of 10-bit PWM).
// CCP1 (CCPR1L) = Left motor (RC2). CCP2 (CCPR2L) = Right motor (RC1).
// -----------------------------------------------------------------------------
void SetMotorSpeed(MotorID motor, uint8_t speed)
{
    uint8_t clamped = (speed > PWM_DUTY_CAP) ? (uint8_t)PWM_DUTY_CAP : speed;

    if(motor == MOTOR_LEFT)
    {
        CCPR1L = clamped;
        CCP1CON = (CCP1CON & 0b11001111);  // DC1B1:DC1B0 = 00 (LSBs of duty cycle)
    }
    else
    {
        CCPR2L = clamped;
        CCP2CON = (CCP2CON & 0b11001111);
    }
}

// -----------------------------------------------------------------------------
// SetMotorDirection()
// Set direction control pins for one motor on the TB6612FNG.
// -----------------------------------------------------------------------------
void SetMotorDirection(MotorID motor, MotorDir dir)
{
    if(motor == MOTOR_LEFT)
    {
        switch(dir)
        {
            case DIR_FORWARD: MOTOR_FORWARD(AIN1, AIN2); break;
            case DIR_REVERSE: MOTOR_REVERSE(AIN1, AIN2); break;
            case DIR_BRAKE:   MOTOR_BRAKE(AIN1, AIN2);   break;
            case DIR_COAST:
            default:          MOTOR_COAST(AIN1, AIN2);   break;
        }
    }
    else
    {
        switch(dir)
        {
            case DIR_FORWARD: MOTOR_FORWARD(BIN1, BIN2); break;
            case DIR_REVERSE: MOTOR_REVERSE(BIN1, BIN2); break;
            case DIR_BRAKE:   MOTOR_BRAKE(BIN1, BIN2);   break;
            case DIR_COAST:
            default:          MOTOR_COAST(BIN1, BIN2);   break;
        }
    }
}

// -----------------------------------------------------------------------------
// Motors_Set()
// Set both motor speeds simultaneously (left and right).
// Directions are left at whatever was previously set (forward by default).
// -----------------------------------------------------------------------------
void Motors_Set(uint8_t left_speed, uint8_t right_speed)
{
    SetMotorSpeed(MOTOR_LEFT,  left_speed);
    SetMotorSpeed(MOTOR_RIGHT, right_speed);
}

// -----------------------------------------------------------------------------
// Motors_Stop()
// Brake both motors (IN1=IN2=1 on TB6612FNG = active brake).
// -----------------------------------------------------------------------------
void Motors_Stop(void)
{
    SetMotorDirection(MOTOR_LEFT,  DIR_BRAKE);
    SetMotorDirection(MOTOR_RIGHT, DIR_BRAKE);
    SetMotorSpeed(MOTOR_LEFT,  0);
    SetMotorSpeed(MOTOR_RIGHT, 0);
}

// =============================================================================
// PID and sensor processing
// =============================================================================

// -----------------------------------------------------------------------------
// PID_ComputeError()
// Compute weighted-average position error from sensor ADC readings.
// Returns signed error (negative = line is left, positive = line is right).
// Writes the number of active (on-line) sensors to *active_count.
//
// Error is scaled: SENSOR_WEIGHT values are *10, so divide result by 10
// before multiplying by gain, or adjust K values accordingly.
// Here gains KP/KI/KD already account for the /10 in main().
// -----------------------------------------------------------------------------
int16_t PID_ComputeError(const uint16_t *sensor_results, uint8_t *active_count)
{
    int32_t weighted_sum = 0;
    uint8_t count = 0;
    uint8_t i;

    for(i = 0; i < SENSOR_COUNT; i++)
    {
        if(sensor_results[i] < LINE_THRESHOLD)
        {
            weighted_sum += (int32_t)SENSOR_WEIGHT[i];
            count++;
        }
    }

    *active_count = count;

    if(count == 0)
        return g_last_error;    // Hold last known error when line is lost

    return (int16_t)(weighted_sum / (int32_t)count);
}

// -----------------------------------------------------------------------------
// Trimmer_ReadBaseSpeed()
// Read the speed trimmer potentiometer (AN6/RE1) and map to BASE_SPEED range.
// ADC result (0-1023) is mapped to (BASE_SPEED_MIN - BASE_SPEED_MAX).
// -----------------------------------------------------------------------------
uint8_t Trimmer_ReadBaseSpeed(void)
{
    uint16_t adc_val  = ADC_Read(TRIMMER_CH);
    // Linear map: [0, 1023] -> [BASE_SPEED_MIN, BASE_SPEED_MAX]
    uint16_t range    = (uint16_t)(BASE_SPEED_MAX - BASE_SPEED_MIN);
    uint8_t  speed    = (uint8_t)(BASE_SPEED_MIN + (uint8_t)((uint32_t)adc_val * range / 1023UL));
    return speed;
}

// =============================================================================
// LED indicator
// =============================================================================

// -----------------------------------------------------------------------------
// LED_Update()
// Write sensor bitmask directly to Port B (RB0-RB5).
// Bit 0 = sensor 1 (far left), bit 5 = sensor 6 (far right).
// LED lights HIGH when sensor detects line (bit is set in mask).
// RB6 and RB7 are masked off to protect ICSP pins.
// -----------------------------------------------------------------------------
void LED_Update(uint8_t sensor_mask)
{
    LED_SET(sensor_mask & 0b00111111);  // Mask RB6 and RB7 - never drive ICSP pins
}

// =============================================================================
// Utility
// =============================================================================

// -----------------------------------------------------------------------------
// Delay_ms()
// Software delay using XC8 built-in __delay_ms().
// Broken into 1ms increments to support values > 255 ms.
// -----------------------------------------------------------------------------
void Delay_ms(uint16_t ms)
{
    while(ms--)
        __delay_ms(1);
}

