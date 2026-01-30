/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

//Define global variables


//Define function prototypes
void configureGPIO();
void configureInputInterupts();
void myTimeDelay(uint32_t delay);
void updateFrequencyState();
void updatePatternState();
void servicePatternState();
void serviceFrequencyState();

//Define typedefs
typedef enum blinkFrequency {One,Two,Four,Zero}; //Frequency in Hz
typedef enum colorPattern {YELLOW,MAGENTA,CYAN,OFF}; //Color pattern

//Set default states
enum blinkFrequency frequencyState = Zero;
enum colorPattern patternState = OFF;

//Main
int main(void)
{
    volatile uint32_t ii;

    // Stop WTD
    MAP_WDT_A_holdTimer();

    configureGPIO();
    configureInputInterupts();

    while (1) {
        //Update from default state / previous state, helper functions defined below main.
        updateFrequencyState();
        updatePatternState();

        //Service states which are updated from updateXState helper functions above.
        servicePatternState();
        serviceFrequencyState();
    }
}

// Delay Function using the ARM Systick timer
// When enabled, the SysTick timer decrements during each clock cycle
// When the count reaches 0, BIT 16 of the CTNL register is set HIGH
// We’ll poll that bit to indicate the time delay is over
void myTimeDelay(uint32_t delay){
    const uint32_t COUNTFLAG = BIT(16);
    MAP_SysTick_setPeriod(delay); // Set Counter Value
    MAP_SysTick_enableModule(); // Turn ON Counter
    // Poll COUNTFLAG until it is ONE
    while((SysTick->CTRL & COUNTFLAG) == 0);
    MAP_SysTick_disableModule(); //Turn OFF Counter and return
}

//configureGPIO
void configureGPIO() {
    //Configure RED LED 1
    P1->DIR |= BIT0; // P1.0 set as output (RED LED 1)
    P1->OUT &= ~BIT0; // P1.0 (RED LED 1) set low

    //Configure RED, GREEN, and BLUE LED 2.
    P2->DIR |= (BIT1 + BIT2 + BIT0); //P2.0 (RED LED 2), P2.1 (GREEN LED 2), and P2.2 (BLUE LED 2) set as output
    P2->OUT &= ~(BIT1 + BIT2 + BIT0); //P2.0 (RED LED 2), P2.1 (GREEN LED 2), and P2.2 (BLUE LED 2) set low on pin

    //Configure S1 and S2
    P1->DIR &= ~(BIT1 + BIT4); // P1.1 (S1) and P1.4 (S2) set as input pin
    P1->REN |= (BIT1 + BIT4); // P1.1 (S1) and P1.4 (S2) enable pull-up/pull-down resistor on pin
    P1->OUT |= (BIT1 + BIT4); // P1.1 (S1) and P1.4 (S2) select pull-up resistor
}

//configureInputInterupts()
void configureInputInterupts() {
    P1->IE |= (BIT1 + BIT4); // P1.1 (S1) and P1.4 (S2) enable interupt on pin
    P1->IES |= (BIT1 + BIT4); //P1.1 (S1) and P1.4 (S2) select high to low interupt edge
    P1->IFG &= ~(BIT1 + BIT4); //P1.1 (S1) and P1.4 (S2) clear interupts
}

void updateFrequencyState() {
    if (P1->IFG & BIT1) {
        P1->IFG &= ~BIT1; // Clear P1.1 (S1) interupt flag
        switch (frequencyState) {
        case Zero:
            frequencyState = One;
            break;
        case One:
            frequencyState = Two;
            break;
        case Two:
            frequencyState = Four;
            break;
        case Four:
            frequencyState = Zero;
            break;
        }
    }
}

void updatePatternState() {
    if (P1->IFG & BIT4) {
        P1->IFG &= ~BIT4; // Clear P1.4 (S2) interupt flag
        switch (patternState) {
        case OFF:
            patternState = YELLOW;
            break;
        case YELLOW:
            patternState = MAGENTA;
            break;
        case MAGENTA:
            patternState = CYAN;
            break;
        case CYAN:
            patternState = OFF;
            break;
            }
        }
}
void servicePatternState() {
    switch (patternState) {
    case OFF:
        P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
        break;
    case YELLOW:
        //P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
        P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
        P2->OUT |= (BIT1 + BIT0); // Turn on RED 2 LED and GREEN LED 2
        break;
    case MAGENTA:
        //P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
        P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
        P2->OUT |= (BIT2 + BIT0); // Turn on RED 2 LED and BLUE LED 2
        break;
    case CYAN:
        //P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
        P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
        P2->OUT |= (BIT2 + BIT1); // Turn on BLUE 2 LED and GREED LED 2
        break;
    }
}

void serviceFrequencyState() {
    switch (frequencyState) {
    case OFF:
        //P1->OUT |= BIT0;
        servicePatternState();
        //myTimeDelay(0);
        //P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
        P1->OUT &= ~BIT0;
        //myTimeDelay(0);
        break;
    case One:
        P1->OUT |= BIT0;
        servicePatternState();
        myTimeDelay(1500000); // Delay 0.5 seconds
        P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
        P1->OUT &= ~BIT0;
        myTimeDelay(1500000); // Delay 0.5 seconds
        break;
    case Two:
        P1->OUT |= BIT0;
        servicePatternState();
        myTimeDelay(1500000/2); // Delay 0.25 seconds
        P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
        P1->OUT &= ~BIT0;
        myTimeDelay(1500000/2); // Delay 0.5 seconds
        break;
    case Four:
        P1->OUT |= BIT0;
        servicePatternState();
        myTimeDelay(1500000/4); // Delay 0.125 seconds
        P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
        P1->OUT &= ~BIT0;
        myTimeDelay(1500000/4); // Delay 0.5 seconds
        break;
    }
}

