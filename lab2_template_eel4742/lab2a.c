/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

/* Function Prototypes */
void configureGPIO(void);
void myTimeDelay(uint32_t delay);

/* Main */
int main(void)
{
    // Stop Watchdog Timer
    MAP_WDT_A_holdTimer();

    configureGPIO();

    while (1)
    {
        // Toggle RED LED1 (P1.0) and RED LED2 (P2.0)
        MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P2, GPIO_PIN0);

        // 0.5 second delay 1 Hz blink rate
        myTimeDelay(1500000);
    }
}

/*
 * Delay Function using the ARM SysTick timer
 * SysTick runs off MCLK = 3 MHz
 */
void myTimeDelay(uint32_t delay)
{
    const uint32_t COUNTFLAG = BIT(16);

    MAP_SysTick_setPeriod(delay);      // Load delay value
    MAP_SysTick_enableModule();        // Start SysTick

    while ((SysTick->CTRL & COUNTFLAG) == 0); // Wait for timer to expire

    MAP_SysTick_disableModule();       // Stop SysTick
}

/* GPIO Configuration */
void configureGPIO(void)
{
    // Configure RED LED1 (P1.0)
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

    // Configure RED LED2 (P2.0)
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
}
