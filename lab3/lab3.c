//******************************************************************************
// Main Comment Header:
// Jacob Bachle
// Current Semester: Spring 2026
// Lab Section: 1
// Date Created: 2/3/26
// S1 changes LED 1 frequency while S2 changes LED 2 colored LED colors while
// toggling at 1 Hz
//******************************************************************************

/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

/* Function Prototypes */
void configureGPIO(void);
void configureInputInterrupts(void);
void configTimer1(uint32_t timer, uint16_t clockPeriod, uint16_t clockDivider, void* callback);
//void configTimer2(uint32_t timer, uint16_t clockPeriod, uint16_t clockDivider);
void servicePatternState(void);
void toggleLEDs(void);
void toggleColored(void);
void pushButtons(void);
void updateFrequencyState(void);
void updatePatternState(void);
void resetTimers(void);

/* Typedefs */
typedef enum blinkFrequency {Zero, One, Two, Four} blinkFrequency;
typedef enum colorPattern {OFF, YELLOW, MAGENTA, CYAN} colorPattern;

/* Timer settings for 0, 1, 2, 4 Hz */
uint16_t clockPeriodArray[]  = {0, 50000/2, 25000/2, 12500/2};
uint16_t clockDividerArray[] = {1, 64, 64, 64};

volatile uint8_t led1Counter = 0;

/* Global State */
blinkFrequency frequencyState = Zero;
colorPattern patternState = OFF;

Timer_A_UpModeConfig timerConfig;

/* Timer Base */
const uint32_t TIMERCOLOR = TIMER_A0_BASE;
const uint32_t TIMER = TIMER_A1_BASE;

//******************************************************************************
// Name of Function: main
// Description: S1 changes LED 1 frequency while S2 changes LED 2 colored LED
// colors while toggling at 1 Hz
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
int main(void) {
    MAP_WDT_A_holdTimer();

    configureGPIO();
    configureInputInterrupts();

    frequencyState = Zero;
    patternState   = OFF;

    configTimer1(TIMER,clockPeriodArray[3],clockDividerArray[3],toggleLEDs);
    configTimer1(TIMERCOLOR,50000/2,64,toggleColored);


    MAP_Interrupt_enableSleepOnIsrExit();

    __enable_interrupts(); //Enable global interrupts, enable GIE bit
    MAP_PCM_gotoLPM0(); //Enter LPM0, timers active, CPU clock inactive

    while (1);   // Required but never executed
}

//******************************************************************************
// Name of Function: configureGPIO
// Description: Configures LED1, LED2, and S1,S2
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void configureGPIO(void) {
    /* LED1 (P1.0) */
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

    /* RGB LED2 (P2.0, P2.1, P2.2) */
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

    /* Switches S1 (P1.1) and S2 (P1.4) */
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1 | GPIO_PIN4);
}

//******************************************************************************
// Name of Function: configureInputInterrupts
// Description: Configures S1 and S2 interupts
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void configureInputInterrupts(void) {
    GPIO_interruptEdgeSelect(GPIO_PORT_P1, GPIO_PIN1 | GPIO_PIN4, GPIO_HIGH_TO_LOW_TRANSITION);

    GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN1 | GPIO_PIN4);

    GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN1 | GPIO_PIN4);

    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN1);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN4);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN1);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN4);
    MAP_GPIO_registerInterrupt(GPIO_PORT_P1, pushButtons);
}

//******************************************************************************
// Name of Function: configTimer1
// Description: Configures timer and calls function once timer counts to CCR0
// Input Parameters: uint32_t timer,uint16_t clockPeriod,uint16_t clockDivider,
// void* callback
// Return: calls function "callback" when timer is reset
// Author: Jacob Bachle
//******************************************************************************
void configTimer1(uint32_t timer,uint16_t clockPeriod,uint16_t clockDivider, void* callback) {
    Timer_A_stopTimer(timer);

    timerConfig.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    timerConfig.clockSourceDivider = clockDivider;
    timerConfig.timerPeriod = clockPeriod;
    timerConfig.timerClear = TIMER_A_DO_CLEAR;

    Timer_A_configureUpMode(timer, &timerConfig);

    Timer_A_clearInterruptFlag(timer);
    Timer_A_enableInterrupt(timer);

    Timer_A_registerInterrupt(timer, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT, callback);

    Timer_A_startCounter(timer, TIMER_A_UP_MODE);
}

//******************************************************************************
// Name of Function: toggleColored
// Description: Toggles colored LED 2
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void toggleColored(void) {
    Timer_A_clearInterruptFlag(TIMERCOLOR);

    if ((P2->OUT & BIT0)|(P2->OUT & BIT1)|(P2->OUT & BIT2)) {
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
    }
    else {
        servicePatternState();
    }
}

//******************************************************************************
// Name of Function: toggleLEDs
// Description: Toggles colored LED 1 at selected frequency
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void toggleLEDs(void) {
    Timer_A_clearInterruptFlag(TIMER);

    if (frequencyState == Zero) {
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
        led1Counter = 0;
        return;
    }

    led1Counter++;

    switch (frequencyState) {
    case One:   // 1 Hz toggle every 4 interrupts
        if (led1Counter >= 4) {
            GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
            led1Counter = 0;
        }
        break;

    case Two:   // 2 Hz toggle every 2 interrupts
        if (led1Counter >= 2) {
            GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
            led1Counter = 0;
        }
        break;

    case Four:  // 4 Hz toggle every interrupt
        GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        led1Counter = 0;
        break;

    default:
        break;
    }
}

//******************************************************************************
// Name of Function: servicePatternState
// Description: Sets LED 2 output based on global current patternState
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void servicePatternState(void) {
    //GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

    switch (patternState) {
    case YELLOW:
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1);
        break;

    case MAGENTA:
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN2);
        break;

    case CYAN:
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1 | GPIO_PIN2);
        break;

    case OFF:
    default:
        break;
    }
}

//******************************************************************************
// Name of Function: pushButtons
// Description: Calls helper functions to update global states
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void pushButtons() {
    updateFrequencyState();
    updatePatternState();
}

//******************************************************************************
// Name of Function: updateFrequencyState
// Description: Updates frequency state based on switch input
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void updateFrequencyState(void) {
    if (GPIO_getInterruptStatus(GPIO_PORT_P1, GPIO_PIN1)) {
        GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN1);

        switch (frequencyState)
        {
        case Zero: frequencyState = One;  break;
        case One:  frequencyState = Two;  break;
        case Two:  frequencyState = Four; break;
        case Four: frequencyState = Zero; break;
        }
        resetTimers();
    }
}

//******************************************************************************
// Name of Function: updatePatternState
// Description: Updates pattern state based on switch input
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void updatePatternState(void) {
    if (GPIO_getInterruptStatus(GPIO_PORT_P1, GPIO_PIN4))
    {
        GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN4);

        switch (patternState) {
        case OFF:     patternState = YELLOW;  break;
        case YELLOW:  patternState = MAGENTA; break;
        case MAGENTA: patternState = CYAN;    break;
        case CYAN:    patternState = OFF;     break;
        }
        resetTimers();
    }
}

//******************************************************************************
// Name of Function: resetTimers
// Description: Reset all timer bases, ensures all timers are in sync
// Input Parameters: void
// Return: none
// Author: Jacob Bachle
//******************************************************************************
void resetTimers(void) {
    // Stop both timers
    Timer_A_stopTimer(TIMER);
    Timer_A_stopTimer(TIMERCOLOR);

    // Clear interrupt flags
    Timer_A_clearInterruptFlag(TIMER);
    Timer_A_clearInterruptFlag(TIMERCOLOR);

    // Clear LEDs
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

    // Reset counters
    led1Counter = 0;

    // Start both timers
    Timer_A_startCounter(TIMER, TIMER_A_UP_MODE);
    Timer_A_startCounter(TIMERCOLOR, TIMER_A_UP_MODE);
}

