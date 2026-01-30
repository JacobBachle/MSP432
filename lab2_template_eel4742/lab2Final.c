/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

/* Function Prototypes */
void configureGPIO(void);
void configureInputInterrupts(void);
void myTimeDelay(uint32_t delay);
void updateFrequencyState(void);
void updatePatternState(void);
void servicePatternState(void);
void serviceFrequencyState(void);

/* Typedefs */
typedef enum blinkFrequency { Zero, One, Two, Four } blinkFrequency;
typedef enum colorPattern { OFF, YELLOW, MAGENTA, CYAN } colorPattern;

/* Global State */
blinkFrequency frequencyState = Zero;
colorPattern patternState = OFF;

/* Main */
int main(void)
{
    MAP_WDT_A_holdTimer();

    configureGPIO();
    configureInputInterrupts();

    while (1)
    {
        updateFrequencyState();
        updatePatternState();

        serviceFrequencyState();
    }
}

void myTimeDelay(uint32_t delay){
    const uint32_t COUNTFLAG = BIT(16);
    MAP_SysTick_setPeriod(delay); // Set Counter Value
    MAP_SysTick_enableModule(); // Turn ON Counter
    // Poll COUNTFLAG until it is ONE
    while((SysTick->CTRL & COUNTFLAG) == 0);
    MAP_SysTick_disableModule(); //Turn OFF Counter and return
}


/* GPIO Configuration */
void configureGPIO(void)
{
    /* RED LED1 (P1.0) */
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

    /* RGB LED2 (P2.0, P2.1, P2.2) */
    GPIO_setAsOutputPin(GPIO_PORT_P2,
        GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2,
        GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

    /* Switches S1 (P1.1) and S2 (P1.4) */
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1,
        GPIO_PIN1 | GPIO_PIN4);
}

/* Interrupt Configuration */
void configureInputInterrupts(void)
{
    GPIO_interruptEdgeSelect(GPIO_PORT_P1,
        GPIO_PIN1 | GPIO_PIN4,
        GPIO_HIGH_TO_LOW_TRANSITION);

    GPIO_clearInterruptFlag(GPIO_PORT_P1,
        GPIO_PIN1 | GPIO_PIN4);

    GPIO_enableInterrupt(GPIO_PORT_P1,
        GPIO_PIN1 | GPIO_PIN4);
}

/* Update Frequency State (S1) */
void updateFrequencyState(void)
{
    if (GPIO_getInterruptStatus(GPIO_PORT_P1, GPIO_PIN1))
    {
        GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN1);

        switch (frequencyState)
        {
        case Zero: frequencyState = One;  break;
        case One:  frequencyState = Two;  break;
        case Two:  frequencyState = Four; break;
        case Four: frequencyState = Zero; break;
        }
    }
}

/* Update Pattern State (S2) */
void updatePatternState(void)
{
    if (GPIO_getInterruptStatus(GPIO_PORT_P1, GPIO_PIN4))
    {
        GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN4);

        switch (patternState)
        {
        case OFF:     patternState = YELLOW;  break;
        case YELLOW:  patternState = MAGENTA; break;
        case MAGENTA: patternState = CYAN;    break;
        case CYAN:    patternState = OFF;     break;
        }
    }
}

/* Service Pattern State */
void servicePatternState(void)
{
    GPIO_setOutputLowOnPin(GPIO_PORT_P2,
        GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

    switch (patternState)
    {
    case OFF:
        break;

    case YELLOW:
        GPIO_setOutputHighOnPin(GPIO_PORT_P2,
            GPIO_PIN0 | GPIO_PIN1);
        break;

    case MAGENTA:
        GPIO_setOutputHighOnPin(GPIO_PORT_P2,
            GPIO_PIN0 | GPIO_PIN2);
        break;

    case CYAN:
        GPIO_setOutputHighOnPin(GPIO_PORT_P2,
            GPIO_PIN1 | GPIO_PIN2);
        break;
    }
}

/* Service Frequency State */
void serviceFrequencyState(void)
{
    uint32_t delay;

    switch (frequencyState)
    {
    case Zero:
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
        servicePatternState();
        break;

    case One:
        delay = 1500000;
        break;

    case Two:
        delay = 1500000 / 2;
        break;

    case Four:
        delay = 1500000 / 4;
        break;

    default:
        return;
    }

    if (frequencyState != Zero)
    {
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
        servicePatternState();
        myTimeDelay(delay);

        GPIO_setOutputLowOnPin(GPIO_PORT_P2,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
        myTimeDelay(delay);
    }
}
