//******************************************************************************
// Main Comment Header:
// Jacob Bachle and Rafael Cano
// Current Semester: Spring 2026
// Lab Section: 1
// Date Created: 3/10/26
// Description:
// Bumper switches control left and right wheel motion using PWM.
// LEDs display status per certification sheet.
//******************************************************************************

// DriverLib Includes
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

// Standard Includes
#include <stdint.h>
#include <stdbool.h>

// Include Lab 9
#include <HALUART432.c>
#include <HALUART432.h>

// Function Prototypes
void toggleAllLEDSONTest(void);
void toggleAllLEDSOFFTest(void);

void toggleRSLKLEDTest(void);

void bumperSwitchesHandler(void);

void configure432IO(void);
void configureRobotIO(void);
//void readBumperSwitches(void);
void serviceLeftMotorLED(void);
void serviceRightMotorLED(void);
void serviceLeftMotor(void);
void serviceRightMotor(void);
void configPWMTimer(uint16_t clockPeriod, uint16_t clockDivider, uint16_t duty, uint16_t channel);

// Functions added in lab 7
void LEDStatesControl(void);
void serviceLEDs(void);
void readLineSensors(void);
void processSensorData(void);

// Functions added in lab 8
void handleStandBy(void);
void updateMotorDuty(void);
void updateMotorDutyL(void);
void updateMotorDutyR(void);
void updateDutyState(void);
void turnOnWhiteLED2(void);
void turnOffWhiteLED2(void);

// Functions added in lab 9
void Encoder_init(void);
uint32_t Encoder_getCount(void);
void Encoder_reset(void);
float Encoder_getRPM(void);

typedef struct Timer_A_initCaptureModeParam
{
    uint8_t captureRegister;
    uint8_t captureMode;
    uint8_t captureInputSelect;
    uint8_t synchronizeCaptureSource;
    uint8_t captureInterruptEnable;
    uint8_t captureOutputMode;
}
Timer_A_initCaptureModeParam;

typedef struct Timer_A_initContinuousModeParam
{
    uint16_t clockSource;           // e.g., TIMER_A_CLOCKSOURCE_SMCLK
    uint16_t clockSourceDivider;    // e.g., TIMER_A_CLOCKSOURCE_DIVIDER_1
    uint16_t timerInterruptEnable;  // e.g., TIMER_A_TAIE_INTERRUPT_DISABLE / ENABLE
    uint16_t startTimer;            // e.g., true/false
}
Timer_A_initContinuousModeParam;

// Global Defines
#define PERIOD 45
//#define DUTY 50
#define slowDuty 5
#define medDuty 10
#define fastDuty 300
#define CLOCKDIVIDER TIMER_A_CLOCKSOURCE_DIVIDER_48
#define LEFTCHANNEL  TIMER_A_CAPTURECOMPARE_REGISTER_4
#define RIGHTCHANNEL TIMER_A_CAPTURECOMPARE_REGISTER_3

// Typedefs to store states
typedef enum {motorOFF,motorForward,motorReverse,motorTest} motorStates;
typedef enum {bumperON, bumperOFF} bumperStates;
typedef enum {RED, YELLOW, GREEN, CYAN, BLUE, WHITE} LED2state;
typedef enum {on, off} LED1state;
typedef enum {PowerUp, StandBy, Tracking, Lost} mode;

// Typedefs added in lab 8
//typedef enum {slow, med, fast} speed;
typedef enum {hardLeft, left, stright, right, hardRight, lost} correctionNeeded;

//Current states
motorStates leftMotorState = motorOFF;
motorStates rightMotorState = motorOFF;
bumperStates BMP0 = bumperOFF;
bumperStates BMPn = bumperOFF;
LED1state LED1State = off;
LED2state LED2State;

mode currentMode = StandBy;
uint8_t currentSpeedL = medDuty;
uint8_t currentSpeedR = medDuty;
correctionNeeded currentCorrectionNeeded = lost;

Timer_A_PWMConfig timerPWMConfig;

// Timer Base
const uint32_t TIMER = TIMER_A0_BASE;

// Global vars
volatile uint8_t sensorValues = 0;
volatile uint8_t leftCount = 0;
volatile uint8_t rightCount = 0;
volatile uint8_t totalCount = 0;

// Initilize encoder count
volatile uint32_t encoderCount = 0;

//Initlizalie timer in capture compare


//******************************************************************************
// Name of Function: main
// Description: Initializes hardware and continuously polls bumper switches
// to control wheel direction and LED indicators.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
int main(void) {
    MAP_WDT_A_holdTimer();

    configure432IO();
    configureRobotIO();

    // Configure PWM for both wheels
    configPWMTimer(PERIOD, CLOCKDIVIDER, currentSpeedL, LEFTCHANNEL);
    configPWMTimer(PERIOD, CLOCKDIVIDER, currentSpeedR, RIGHTCHANNEL);

    serviceLeftMotor();
    serviceRightMotor();

    //toggleRSLKLEDTest();

    Timer_A_startCounter(TIMER, TIMER_A_UP_MODE);

    //__enable_interrupts();

    //Power up
    toggleRSLKLEDTest(); //Add toggle LED 1 and LED 2

    while(1) {
        if (currentMode == StandBy) {

            //MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

            //P1->OUT |= BIT0;
            handleStandBy();
            updateDutyState();

            //Wheels off
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7 | GPIO_PIN6);

            //Check BMP
            if (GPIO_getInterruptStatus(GPIO_PORT_P4, GPIO_PIN0)) {
                GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN0);
                currentMode = Tracking;
                //LED 1 on
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
                //Set motor states

                //Wheels on
                // Left wheel
                // IN1 = 1, IN2 = 0
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);

                // set direction
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN4);

                // Enable motor
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);

                //Right wheel
                // IN1 = 1, IN2 = 0
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2);

                // set direction
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN5);

                // Enable motor
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6);
            }
            //__delay_cycles(1500000);
            //P1->OUT &= ~BIT0;
        }
        else if (currentMode == Tracking) {
            handleStandBy();
            updateDutyState();

            //updateMotorDuty
            Timer_A_stopTimer(TIMER);
            configPWMTimer(PERIOD, CLOCKDIVIDER, currentSpeedR, LEFTCHANNEL);
            configPWMTimer(PERIOD, CLOCKDIVIDER, currentSpeedL, RIGHTCHANNEL);
            Timer_A_startCounter(TIMER, TIMER_A_UP_MODE);

// MAP_GPIO_getInputPinValue(GPIO_PIN0 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
            //Check BMP
            if ((MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN0) == 0) | (MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN2) == 0) | (MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN3) == 0) | (MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN5) == 0) | (MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN6) == 0) | (MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN7) == 0)) {
                currentMode = StandBy;
            }
        }
        else if (currentMode == Lost) {
            //LED 1 off
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
            //LED 2
            readLineSensors();
            processSensorData();
            LEDStatesControl();
            serviceLEDs();
            __delay_cycles(1500000); //Delay 500 ms

            //RSLK wheels active
            leftMotorState = motorReverse;
            rightMotorState = motorReverse;

            serviceLeftMotor();
            serviceRightMotor();

        }
    }
}

//******************************************************************************
// Name of Function: configure432IO
// Description: Configures LaunchPad LED1 and RGB LED2 as outputs
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void configure432IO(void) {
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
}

//******************************************************************************
// Name of Function: configureRobotIO
// Description: Configures bumper switches, motor pins, and PWM outputs
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void configureRobotIO(void) {
    // Configure IR LED control pins as outputs
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN3);  // Even LEDs
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3); // Start OFF

    MAP_GPIO_setAsOutputPin(GPIO_PORT_P9, GPIO_PIN2);  // Odd LEDs
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P9, GPIO_PIN2); // Start OFF

    // Bumper switches config
    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN7 | GPIO_PIN6 | GPIO_PIN5 | GPIO_PIN3 | GPIO_PIN2 | GPIO_PIN0);

    // Direction pins
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN5 | GPIO_PIN4 | GPIO_PIN2 | GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN5 | GPIO_PIN4 | GPIO_PIN2 | GPIO_PIN0); //Default low

    // Sleep/Enable pins
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN7 | GPIO_PIN6);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7 | GPIO_PIN6); //Default low

    // PWM outputs
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN6, GPIO_PRIMARY_MODULE_FUNCTION);

    // RSLK LED config
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7); //Default off

    //enable the port I/O interrupt for the switch
    GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN0);
    GPIO_interruptEdgeSelect(GPIO_PORT_P4, GPIO_PIN0, GPIO_LOW_TO_HIGH_TRANSITION);
    GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN0);
    //MAP_GPIO_registerInterrupt(GPIO_PORT_P4, bumperSwitchesHandler);

}

//******************************************************************************
// Name of Function: serviceRightMotor
// Description: sets right motor configuration bits based on current right motor
// state.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void serviceRightMotor(void) {
    switch(rightMotorState) {
        case motorOFF:
            // Disable motor
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN6);

            // Direction pins low
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0 | GPIO_PIN2);
            break;

        case motorForward:
            // IN1 = 1, IN2 = 0
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2);

            // set direction
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN5);

            // Enable motor
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6);
            break;

        case motorReverse:
            // IN1 = 0, IN2 = 1
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2);

            // set direction
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN5);

            // Enable motor
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6);
            break;

        case motorTest:
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0 | GPIO_PIN2);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6);
            break;
    }
}

//******************************************************************************
// Name of Function: serviceLeftMotor
// Description: sets left motor configuration bits based on current left motor
// state.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void serviceLeftMotor(void) {
    switch(leftMotorState) {
        case motorOFF:
            // Disable motor
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7);

            // Direction pins low
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN4 | GPIO_PIN5);
            break;

        case motorForward:
            // IN1 = 1, IN2 = 0
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);

            // set direction
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN4);

            // Enable motor
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);
            break;

        case motorReverse:
            // IN1 = 0, IN2 = 1
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);

            // set direction
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN4);

            // Enable motor
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);
            break;

        case motorTest:
            // Both direction high (optional test mode)
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN4 | GPIO_PIN5);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);
            break;
    }
}

//******************************************************************************
// Name of Function: serviceLeftMotorLED
// Description: Sets RSLK LEDs high or low based on current motor states.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void serviceLeftMotorLED(void) {
    switch(leftMotorState) {
    case motorOFF:
        // RSLK LED Front and Rear OFF
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0); // Front Left OFF
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN6); // Rear Left OFF
        break;
    case motorForward:
        // RSLK LED Front ON Rear OFF
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN0); // Front Left ON
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN6);  // Rear Left OFF
        break;
    case motorReverse:
        // RSLK LED Rear ON Front OFF
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0);  // Front Left OFF
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN6); // Rear Left ON
        break;
    case motorTest:
        //RSLK LED Front and Rear ON
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN0);
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN6);
        break;
    }
}

//******************************************************************************
// Name of Function: serviceRightMotorLED
// Description: Sets RSLK LEDs high or low based on current motor states.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void serviceRightMotorLED(void) {
    switch(rightMotorState) {
    case motorOFF:
        // RSLK LED Front and Rear OFF
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN5); // Front Left OFF
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN7); // Rear Left OFF
        break;
    case motorForward:
        // RSLK LED Front ON Rear OFF
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN5); // Front Left ON
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN7);  // Rear Left OFF
        break;
    case motorReverse:
        // RSLK LED Rear ON Front OFF
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN5);  // Front Left OFF
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN7); // Rear Left ON
        break;
    case motorTest:
        //RSLK LED Front and Rear ON
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN5);
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN7);
        break;
    }
}

//******************************************************************************
// Name of Function: toggleAllLEDSONTest
// Description: Turns all LEDS on. Used as a helper function in test mode.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void toggleAllLEDSONTest(void) {
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
}

//******************************************************************************
// Name of Function: toggleAllLEDSOFFTest
// Description: Turns all LEDS off. Used as a helper function in test mode.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void toggleAllLEDSOFFTest(void) {
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
}

//******************************************************************************
// Name of Function: toggleRSLKLEDTest
// Description: Turns all LEDS of, after a 3 second delay, turns all LEDs off.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void toggleRSLKLEDTest(void) {
    turnOnWhiteLED2();
    toggleAllLEDSONTest();
    __delay_cycles(9000000); //aprox. 3 seconds
    toggleAllLEDSOFFTest();
    turnOffWhiteLED2();
}

//******************************************************************************
// Name of Function: bumperSwitchesHandler
// Description: Updates bumper states.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void bumperSwitchesHandler(void) {
    uint16_t status;
    _delay_cycles(30000);
    status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P4);
    switch(status){
        case GPIO_PIN6: //BPM5
            if (BMPn == bumperOFF) {
                BMPn = bumperON;
            }
            else {
                BMPn = bumperOFF;
            }
            //GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
            break;
        case GPIO_PIN5: //BMP4
            if (BMPn == bumperOFF) {
                BMPn = bumperON;
            }
            else {
                BMPn = bumperOFF;
            }
            //GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN5);
            break;
        case GPIO_PIN4: //BPM3
            if (BMPn == bumperOFF) {
                BMPn = bumperON;
            }
            else {
                BMPn = bumperOFF;
            }
            //GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN4);
            break;
        case GPIO_PIN3: //BMP2
            if (BMPn == bumperOFF) {
                BMPn = bumperON;
            }
            else {
                BMPn = bumperOFF;
            }
            //GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN3);
            break;
        case GPIO_PIN2: //BMP1
            if (BMPn == bumperOFF) {
                BMPn = bumperON;
            }
            else {
                BMPn = bumperOFF;
            }
            //GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN2);
            break;
        case GPIO_PIN0: // BMP0
            if (BMP0 == bumperOFF) {
                BMP0 = bumperON;
            }
            else {
                BMP0 = bumperOFF;
            }
            //GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN0);
            break;
        //End of Case
    }

    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P4, status);

    //readBumperSwitches();
}

//******************************************************************************
// Name of Function: processSensorData
// Description: Counts sensor values to count how many senesors indicate right
// and left activation.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void processSensorData(void) {
    leftCount = 0;
    rightCount = 0;
    if ((sensorValues & BIT0)) {
        leftCount++;
    }
    if ((sensorValues & BIT1)) {
        leftCount++;
    }
    if ((sensorValues & BIT2)) {
        leftCount++;
    }
    if ((sensorValues & BIT3)) {
        leftCount++;
    }

    if ((sensorValues & BIT4)) {
        rightCount++;
    }
    if ((sensorValues & BIT5)) {
        rightCount++;
    }
    if ((sensorValues & BIT6)) {
        rightCount++;
    }
    if ((sensorValues & BIT7)) {
        rightCount++;
    }
}

//******************************************************************************
// Name of Function: LEDStatesControl
// Description: Using processed sensor data, to determine the positon the RSLK
// is on the track mapped to an LED2 state.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void LEDStatesControl(void) {
    totalCount = (leftCount + rightCount);

    if (totalCount == 0) {
        LED2State = WHITE;
        currentCorrectionNeeded = lost;
    }
    else if (leftCount == rightCount) {
        LED2State = GREEN;
        currentCorrectionNeeded = stright;
    }
    else if ((leftCount > 0) & (rightCount == 0)) {
        LED2State = RED;
        currentCorrectionNeeded = hardRight;
    }
    else if ((rightCount > 0) & (leftCount == 0)) {
        LED2State = BLUE;
        currentCorrectionNeeded = hardLeft;
    }
    else if (leftCount > rightCount) {
        LED2State = YELLOW;
        currentCorrectionNeeded = right;
    }
    else if (rightCount > leftCount) {
        LED2State = CYAN;
        currentCorrectionNeeded = left;
    }

    /*
    // LED1 mode control
    switch(currentMode) {
        case PowerUp:
            // Toggle LED1 once
            GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
            __delay_cycles(1500000); //Delay 500 ms
            GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);

            // Toggle LED2 white once
            GPIO_toggleOutputOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
            __delay_cycles(1500000); //Delay 500 ms
            GPIO_toggleOutputOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
            break;
        case StandBy:
            if (totalCount > 0) {
                //Toggle LED1 at 1 Hz
                GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
                __delay_cycles(1500000); //Delay 500 ms
            }
            else {
                LED1State = off;
            }
            break;
        case Tracking:
            LED1State = on;
            break;
        case Lost:
            LED1State = off;
            break;
    }
    */
}

//******************************************************************************
// Name of Function: turnOnWhiteLED2
// Description: ToggleLED helper function. Turns on white led
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void turnOnWhiteLED2(void) {
    P1->OUT |= BIT0;
    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
}

//******************************************************************************
// Name of Function: turnOffWhiteLED2
// Description: ToggleLED helper function. Turns off white led
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void turnOffWhiteLED2(void) {
    P1->OUT &= ~BIT0;
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
}

//******************************************************************************
// Name of Function: serviceLEDs
// Description: Updates LED2 to match the LED2 state.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void serviceLEDs(void) {
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2); //Sets all LED pins to low
    switch(LED1State) {
    case on:
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
        break;
    case off:
        //Turn off LED1, already off
        break;
    }
    switch(LED2State) {
    case WHITE:
        //Turn on white
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
        break;
    case RED:
        //Turn on red
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
        break;
    case BLUE:
        //Turn on blue
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
        break;
    case YELLOW:
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1);
        break;
    case CYAN:
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1 | GPIO_PIN2);
        break;
    case GREEN:
        //turn on green
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
        break;
    }
}

//******************************************************************************
// Name of Function: readLineSensors
// Description: Configures Timer_A PWM channel for selected wheel
// Input Parameters: clockPeriod, clockDivider, duty cycle %, channel
// Return: none
// Author: Rafael Cano
//******************************************************************************
void readLineSensors(void) {
    sensorValues = 0;
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN3);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P9, GPIO_PIN2);

    MAP_GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P7, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    __delay_cycles(30);
    MAP_GPIO_setAsInputPin(GPIO_PORT_P7, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    __delay_cycles(3000);
    //sensorValues = MAP_GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);

    if (MAP_GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN0)) {
        sensorValues |= BIT0;
    }
    if (MAP_GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN1)) {
        sensorValues |= BIT1;
    }
    if (MAP_GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN2)) {
        sensorValues |= BIT2;
    }
    if (MAP_GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN3)) {
        sensorValues |= BIT3;
    }
    if (MAP_GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN4)) {
        sensorValues |= BIT4;
    }
    if (MAP_GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN5)) {
        sensorValues |= BIT5;
    }
    if (MAP_GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN6)) {
        sensorValues |= BIT6;
    }
    if (MAP_GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN7)) {
        sensorValues |= BIT7;
    }

    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P9, GPIO_PIN2);
    //return data;
}

//******************************************************************************
// Name of Function: handleStandBy
// Description: Uses helper function to read and process sensor data to update
// LED2 state.
// Return: none
// Author: Rafael Cano
//******************************************************************************
void handleStandBy(void) {
    readLineSensors();
    processSensorData();
    LEDStatesControl();
    serviceLEDs();
    __delay_cycles(30000); //Delay 500 ms
}

//******************************************************************************
// Name of Function: updateDutyState
// Description: Updates the duty of the left and right based on the correction
// needed.
// Return: none
// Author: Rafael Cano
//******************************************************************************
void updateDutyState(void) {
    switch (currentCorrectionNeeded) {
    case hardLeft:
        currentSpeedL = fastDuty;
        currentSpeedR = slowDuty;
        break;
    case left:
        currentSpeedL = medDuty;
        currentSpeedR = slowDuty;
        break;
    case stright:
        currentSpeedL = medDuty;
        currentSpeedR = medDuty;
        break;
    case hardRight:
        currentSpeedL = slowDuty;
        currentSpeedR = fastDuty;
        break;
    case right:
        currentSpeedL = slowDuty;
        currentSpeedR = medDuty;
        break;
    case lost:
        currentSpeedL = slowDuty;
        currentSpeedR = slowDuty;
        break;
    }
}

void Encoder_init(void) {
    // Left Encoder A
    MAP_GPIO_setAsInputPin(GPIO_PORT_P10, GPIO_PIN5);

    MAP_GPIO_interruptEdgeSelect(GPIO_PORT_P10, GPIO_PIN5, GPIO_LOW_TO_HIGH_TRANSITION);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P10, GPIO_PIN5);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P10, GPIO_PIN5);

    // Right Encoder A
    MAP_GPIO_setAsInputPin(GPIO_PORT_P10, GPIO_PIN4);

    MAP_GPIO_interruptEdgeSelect(GPIO_PORT_P10, GPIO_PIN4, GPIO_LOW_TO_HIGH_TRANSITION);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P10, GPIO_PIN4);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P10, GPIO_PIN4);

    // Enable GIE
    __enable_interrupts();

    // Left Encoder B
    MAP_GPIO_

    // Right Encoder B

}

void PORT10_IRQHandler(void) {
    uint32_t status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P10);

    if (status & GPIO_PIN5) {
        encoderCount++;
        MAP_GPIO_clearInterruptFlag(GPIO_PORT_P10, GPIO_PIN5);
    }


    /**
    if (status & GPIO_PIN4) {

    }
    **/
}

uint32_t Encoder_getCount(void) {

}

void Encoder_reset(void) {

}

float Encoder_getRPM(void) {

}

//******************************************************************************
// Name of Function: configPWMTimer
// Description: Configures Timer_A PWM channel for selected wheel
// Input Parameters: clockPeriod, clockDivider, duty cycle %, channel
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void configPWMTimer(uint16_t clockPeriod, uint16_t clockDivider, uint16_t duty, uint16_t channel) {
    uint16_t dutyCycle = duty * clockPeriod / 100;
    timerPWMConfig.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    timerPWMConfig.clockSourceDivider = clockDivider;
    timerPWMConfig.timerPeriod = clockPeriod;
    timerPWMConfig.compareOutputMode =
    TIMER_A_OUTPUTMODE_TOGGLE_SET;
    timerPWMConfig.compareRegister = channel;
    timerPWMConfig.dutyCycle = dutyCycle;

    MAP_Timer_A_generatePWM(TIMER, &timerPWMConfig);
    MAP_Timer_A_stopTimer(TIMER);
}
