//******************************************************************************
//   MSP432P401 Demo - Software Toggle P1.0
//
//   Description: Toggle P1.0 by xor'ing P1.0 inside of a software loop.
//   ACLK = 32.768kHz, MCLK = SMCLK = default DCO~1MHz
//
//                MSP432P401x
//             -----------------
//         /|\|                 |
//          | |                 |
//          --|RST              |
//            |                 |
//            |             P1.0|-->LED
//
//   William Goh
//   Texas Instruments Inc.
//   June 2016 (updated) | November 2013 (created)
//   Built with CCSv6.1, IAR, Keil, GCC
//******************************************************************************
#include "ti/devices/msp432p4xx/inc/msp.h"
#include <stdint.h>


int main(void) {
    volatile uint32_t i;

    WDT_A->CTL = WDT_A_CTL_PW |             // Stop WDT
                 WDT_A_CTL_HOLD;

    P1->DIR |= BIT0;                        // P1.0 set as output (RED LED 1)
    P2->DIR |= BIT0;                        // P2.0 set as output (RED LED 2)
    P1->OUT &= ~BIT0; // P1.0 (RED LED 1) set low
    P2->OUT &= ~BIT0; // P2.0 (RED LED 2) set low

    P2->DIR |= (BIT1 + BIT2); //P2.1 (GREEN LED 2) and P2.2 (BLUE LED 2) set as output
    P2->OUT &= ~(BIT1 + BIT2); //P2.1 (GREEN LED 2) and P2.2 (BLUE LED 2) set low on pin

    P1->DIR &= ~(BIT1 + BIT4); // P1.1 (S1) and P1.4 (S2) set as input pin
    P1->REN |= (BIT1 + BIT4); // P1.1 (S1) and P1.4 (S2) enable pull-up/pull-down resistor on pin
    P1->OUT |= (BIT1 + BIT4); // P1.1 (S1) and P1.4 (S2) select pull-up resistor

    P1->IE |= (BIT1 + BIT4); // P1.1 (S1) and P1.4 (S2) enable interupt on pin
    P1->IES |= (BIT1 + BIT4); //P1.1 (S1) and P1.4 (S2) select high to low interupt edge
    P1->IFG &= ~(BIT1 + BIT4); //P1.1 (S1) and P1.4 (S2) clear interupts

    typedef enum blinkFrequency {One,Two,Four,Zero}; //Frequency in Hz
    typedef enum colorPattern {RED,GREEN,BLUE,OFF}; //Color pattern

    enum blinkFrequency frequencyState = Zero;
    enum colorPattern patternState = OFF;

    while (1)                               // continuous loop
    {
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

        if (P1->IFG & BIT4) {
            P1->IFG &= ~BIT4; // Clear P1.4 (S2) interupt flag
            switch (patternState) {
            case OFF:
                patternState = RED;
                break;
            case RED:
                patternState = GREEN;
                break;
            case GREEN:
                patternState = BLUE;
                break;
            case BLUE:
                patternState = OFF;
                break;
                }
            }

        switch (patternState) {
        case OFF:
            P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
            break;
        case RED:
            P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
            P2->OUT |= BIT0; // Turn on RED 2 LED
            break;
        case GREEN:
            P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
            P2->OUT |= BIT1; // Turn on GREEN 2 LED
            break;
        case BLUE:
            P2->OUT &= ~(BIT0 + BIT1 + BIT2); // Turn off all LED 2
            P2->OUT |= BIT2; // Turn on BLUE 2 LED
            break;
        }

        switch (frequencyState) {
        case OFF:
            P1->OUT &= ~BIT0;
            break;
        case One:
            P1->OUT ^= BIT0;
            __delay_cycles(1500000);             // Delay 0.5 seconds
            break;
        case Two:
            P1->OUT ^= BIT0;
            __delay_cycles(1500000/2);             // Delay 0.25 seconds
            break;
        case Four:
            P1->OUT ^= BIT0;
            __delay_cycles(1500000/4);             // Delay 0.125 seconds
            break;
        }

    }
}

