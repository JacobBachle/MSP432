//******************************************************************************
// Main Comment Header:
// Jacob Bachle and Rafael Cano
// Current Semester: Spring 2026
// Lab Section: 1
// Date Created: 2/12/26
// Description:
// Bumper switches control left and right wheel motion using PWM.
// LEDs display status per certification sheet.
//******************************************************************************

// DriverLib Includes
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

// Standard Includes
#include <stdint.h>
#include <stdbool.h>

// Function Prototypes
void configure432IO(void);
void configureRobotIO(void);
void readBumperSwitches(void);
void configPWMTimer(uint16_t clockPeriod, uint16_t clockDivider, uint16_t duty, uint16_t channel);

// Global Defines
#define PERIOD 2000 //Determined in earlier part of lab to be PERIOD in which motor would appear continuous
#define DUTY 50
#define CLOCKDIVIDER TIMER_A_CLOCKSOURCE_DIVIDER_48
#define LEFTCHANNEL  TIMER_A_CAPTURECOMPARE_REGISTER_4
#define RIGHTCHANNEL TIMER_A_CAPTURECOMPARE_REGISTER_3

// Typedefs to store states
typedef enum {LWOff, LWOn} LWState;
typedef enum {RWOff, RWOn} RWState;

//Global Variables
LWState LeftWheelState  = LWOff;
RWState RightWheelState = RWOff;

Timer_A_PWMConfig timerPWMConfig;

// Timer Base
const uint32_t TIMER = TIMER_A0_BASE;

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
    configPWMTimer(PERIOD, CLOCKDIVIDER, DUTY, LEFTCHANNEL);
    configPWMTimer(PERIOD, CLOCKDIVIDER, DUTY, RIGHTCHANNEL);

    Timer_A_startCounter(TIMER, TIMER_A_UP_MODE);

    while(1) {
        readBumperSwitches();

        /* Control Left Wheel */
        if(LeftWheelState == LWOn)
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);
        else
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7);

        /* Control Right Wheel */
        if(RightWheelState == RWOn) {
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6);
        }
        else {
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN6);
        }

        __delay_cycles(1500000); //Software delay for simplicity, could configure timer with seperate base
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
    // Bumper switches
    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN7 | GPIO_PIN6 | GPIO_PIN5 | GPIO_PIN3 | GPIO_PIN2 | GPIO_PIN0);

    // Direction pins
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN5 | GPIO_PIN4 | GPIO_PIN2 | GPIO_PIN0);

    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN5 | GPIO_PIN4 | GPIO_PIN2 | GPIO_PIN0);

    // Sleep/Enable pins
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN7 | GPIO_PIN6);

    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7 | GPIO_PIN6);

    // PWM outputs
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN6, GPIO_PRIMARY_MODULE_FUNCTION);
}

//******************************************************************************
// Name of Function: readBumperSwitches
// Description: Reads bumper switches and updates wheel states and LEDs
// according to certification sheet.
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void readBumperSwitches(void) {
    // Clear LEDs
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN1 | GPIO_PIN0);

    LeftWheelState  = LWOff;
    RightWheelState = RWOff;

    // BMP5
    if((MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN7) == GPIO_INPUT_PIN_LOW)) {
        LeftWheelState = LWOn;

        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
    }

    // BMP4
    else if((MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN6) == GPIO_INPUT_PIN_LOW)) {
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
    }

    // BMP3
    else if((MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN5) == GPIO_INPUT_PIN_LOW)) {
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
    }

    // BMP2
    else if((MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN3) == GPIO_INPUT_PIN_LOW)) {
        LeftWheelState  = LWOn;
        RightWheelState = RWOn;

        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
    }

    // BMP1
    else if((MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN2) == GPIO_INPUT_PIN_LOW)) {
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
    }

    // BMP0
    else if((MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN0) == GPIO_INPUT_PIN_LOW)) {
        RightWheelState = RWOn;
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
    }
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
