//******************************************************************************
// Main Comment Header:
// Your Name
// Current Semester: Spring 2026
// Lab Section: 1
// Date Created: 4/1/26
// Description: Measures right wheel RPM using encoder feedback. Bumper press
// enables motor and prints RPM over UART.
//******************************************************************************

// DriverLib Includes
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

// Standard Includes
#include <stdint.h>
#include <stdio.h>

#include "HALUART432.h"

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
void configureGPIO(void);
void Motor_Init(void);
void Motor_SetRightSpeed(uint16_t duty);
void Motor_Stop(void);

//------------------------------------------------------------------------------
// Globals Variables
//------------------------------------------------------------------------------
#define PPR 360
#define PWM_PERIOD 300
#define TA_RPM 120

volatile uint32_t pulseCount = 0;

// PWM config structure define
Timer_A_PWMConfig rightMotorPWMConfig = {
    TIMER_A_CLOCKSOURCE_SMCLK,
    TIMER_A_CLOCKSOURCE_DIVIDER_1,
    PWM_PERIOD,
    TIMER_A_CAPTURECOMPARE_REGISTER_3,
    TIMER_A_OUTPUTMODE_RESET_SET,
    0
};

//*******************************************************************************
// Name of Function: main
// Description: Initializes system, reads encoder pulses, and calculates RPM
// based on bumper input.
// Input Parameters: void
// Return: none
//*******************************************************************************
int main(void) {
    WDT_A_holdTimer();

    UART_init();
    configureGPIO();
    Motor_Init();

    Interrupt_enableMaster();

    // Power-up LED test
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
    __delay_cycles(3000000);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

    Motor_Stop();

    while (1) {
        if (GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN0) == 0) {

            // LED2 RED
            GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1 | GPIO_PIN2);

            // Run motor
            Motor_SetRightSpeed(TA_RPM);

            // Measure RPM
            pulseCount = 0;
            __delay_cycles(3000000);

            uint32_t currentCount = pulseCount;
            uint32_t currentRPM = currentCount / 6; // 6 stators

            // UART output
            char txBuffer[50];
            sprintf(txBuffer, "Right Wheel Speed: %lu RPM\r\n", currentRPM);
            UART_transmitString(txBuffer);

            // Toggle LED1
            GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        }
        else {
            Motor_Stop();

            GPIO_setOutputLowOnPin(GPIO_PORT_P2,
                GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

            GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
            __delay_cycles(600000);
        }
    }
}

//*******************************************************************************
// Name of Function: configureGPIO
// Description: Configures all GPIO including LEDs, encoder inputs,
// bumper switch, and interrupts.
// Input Parameters: void
// Return: none
//*******************************************************************************
void configureGPIO(void) {

    // Encoder inputs
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P10, GPIO_PIN4);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P5, GPIO_PIN0);

    // Interrupt on Encoder B (P5.0)
    GPIO_interruptEdgeSelect(GPIO_PORT_P5, GPIO_PIN0, GPIO_LOW_TO_HIGH_TRANSITION);
    GPIO_clearInterruptFlag(GPIO_PORT_P5, GPIO_PIN0);
    GPIO_enableInterrupt(GPIO_PORT_P5, GPIO_PIN0);
    Interrupt_enableInterrupt(INT_PORT5);

    // Bumper switch
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN0);

    // LED1
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

    // RGB LED2
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
}

//*******************************************************************************
// Name of Function: Motor_Init
// Description: Initializes motor PWM and control pins.
// Input Parameters: void
// Return: none
//*******************************************************************************
void Motor_Init(void) {

    // PWM output (P2.6)
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,
        GPIO_PIN6, GPIO_PRIMARY_MODULE_FUNCTION);

    // Direction pins
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN4 | GPIO_PIN5);
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN5);
    GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN4);

    // Enable pins
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN6 | GPIO_PIN7);
    GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6 | GPIO_PIN7);

    Timer_A_generatePWM(TIMER_A0_BASE, &rightMotorPWMConfig);
}

//*******************************************************************************
// Name of Function: Motor_SetRightSpeed
// Description: Sets PWM duty cycle for right motor.
// Input Parameters: duty
// Return: none
//*******************************************************************************
void Motor_SetRightSpeed(uint16_t duty) {
    if (duty > PWM_PERIOD) {
        duty = PWM_PERIOD;
    }

    Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3, duty);
}

//*******************************************************************************
// Name of Function: Motor_Stop
// Description: Stops the motor by setting duty cycle to zero.
// Input Parameters: void
// Return: none
//*******************************************************************************
void Motor_Stop(void) {
    Motor_SetRightSpeed(0);
}

//*******************************************************************************
// Name of Function: PORT5_IRQHandler
// Description: Interrupt handler for encoder pulse counting.
// Input Parameters: void
// Return: none
//*******************************************************************************
void PORT5_IRQHandler(void) {
    uint32_t status = GPIO_getEnabledInterruptStatus(GPIO_PORT_P5);
    GPIO_clearInterruptFlag(GPIO_PORT_P5, status);

    if (status & GPIO_PIN0) {
        pulseCount++;
    }
}
