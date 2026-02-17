/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

/* Global Defines */
#define PERIOD 2000
#define DUTY 50
#define CLOCKDIVIDER TIMER_A_CLOCKSOURCE_DIVIDER_48
#define LEFTCHANNEL TIMER_A_CAPTURECOMPARE_REGISTER_4
#define RIGHTCHANNEL TIMER_A_CAPTURECOMPARE_REGISTER_3

/* Global Variables */
Timer_A_PWMConfig timerPWMConfig;
typedef enum {LWOff, LWOn} LWState;
LWState LeftWheelState = LWOff;
typedef enum {RWOff, RWOn} RWState;
RWState RightWheelState = RWOff;

const uint32_t TIMER = TIMER_A0_BASE;

/* Function Declarations */
void config432IO();
void configRobotIO();
void readBumperSwitches();
void configPWMTimer(uint16_t clockPeriod, uint16_t clockDivider, uint16_t duty, uint16_t channel);

//![Simple GPIO Config]
int main(void)
{
    /* Halting the Watchdog */
    MAP_WDT_A_holdTimer();

    config432IO();
    configRobotIO();

    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN4);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7);

    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN5);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN6);

    configPWMTimer(PERIOD, CLOCKDIVIDER, DUTY, LEFTCHANNEL);
    configPWMTimer(PERIOD, CLOCKDIVIDER, DUTY, RIGHTCHANNEL);

    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    while(1){
        readBumperSwitches();
        if(LeftWheelState == LWOn){
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);
        }
        else MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7);

        if(RightWheelState == RWOn){
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6);
        }
        else MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN6);

        __delay_cycles(1500000);

    }
}

void config432IO(){
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN1 | GPIO_PIN0);

    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN1 | GPIO_PIN0);

    __delay_cycles(1500000);

    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN1 | GPIO_PIN0);
}

void configRobotIO(){
    // Bumper Switches
    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN7 | GPIO_PIN6 | GPIO_PIN5 | GPIO_PIN3 | GPIO_PIN2 | GPIO_PIN0);

    // Left and Right Wheel Enable and Direction
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN5 | GPIO_PIN4 | GPIO_PIN2| GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN5 | GPIO_PIN4 | GPIO_PIN2| GPIO_PIN0);
    // Left and Right Wheel Sleep
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN7 | GPIO_PIN6);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7 | GPIO_PIN6);
    // Left and Right Wheel PWM
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN6, GPIO_PRIMARY_MODULE_FUNCTION);
}

void readBumperSwitches(){
    if (MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN7) == GPIO_INPUT_PIN_LOW){
        LeftWheelState = LWOn;
        RightWheelState = RWOff;
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);

        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN1);
    }
    else if(MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN6) == GPIO_INPUT_PIN_LOW){
        LeftWheelState = LWOff;
        RightWheelState = RWOff;
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);

        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN0);
    }
    else if(MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN5) == GPIO_INPUT_PIN_LOW){
        LeftWheelState = LWOff;
        RightWheelState = RWOff;
        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);

        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1 | GPIO_PIN0);
    }
    else if(MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN3) == GPIO_INPUT_PIN_LOW){
        LeftWheelState = LWOn;
        RightWheelState = RWOn;
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1 | GPIO_PIN0);
    }
    else if(MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN2) == GPIO_INPUT_PIN_LOW){
        LeftWheelState = LWOff;
        RightWheelState = RWOff;
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN0);
    }
    else if(MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN0) == GPIO_INPUT_PIN_LOW){
        LeftWheelState = LWOff;
        RightWheelState = RWOn;
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

        MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN1);
    }
    else {
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
        MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN1 | GPIO_PIN0);
        LeftWheelState = LWOff;
        RightWheelState = RWOff;
    }
}

void configPWMTimer(uint16_t clockPeriod, uint16_t clockDivider, uint16_t duty, uint16_t channel){
    const uint32_t TIMER = TIMER_A0_BASE;
    uint16_t dutyCycle = duty*clockPeriod/100;
    timerPWMConfig.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    timerPWMConfig.clockSourceDivider = clockDivider;
    timerPWMConfig.timerPeriod = clockPeriod;
    timerPWMConfig.compareOutputMode = TIMER_A_OUTPUTMODE_TOGGLE_SET;
    timerPWMConfig.compareRegister = channel;
    timerPWMConfig.dutyCycle = dutyCycle;
    MAP_Timer_A_generatePWM(TIMER, &timerPWMConfig);
    MAP_Timer_A_stopTimer(TIMER);
}
