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

    //CS->KEY = CS_KEY_VAL;
    //CS->CTL0 = 0;
    //CS->CTL0 = CS_CTL0_DCORSEL_1;           // 3 MHz DCO
    //CS->CTL1 = CS_CTL1_SELM__DCOCLK;        // MCLK = DCO
    //CS->KEY = 0;

    P1->DIR |= BIT0;                        // P1.0 set as output (RED LED 1)
    P1->DIR |= BIT2;                        // P1.2 set as output (BLUE LED 1)

    P1->DIR &= ~(BIT0 + BIT2); // P1.0 and P1.2 set as to low (RED LED 1, BLUE LED 1)

    P1->DIR &= ~(BIT1 + BIT4); // P1.1 and P1.4 set as input (S1, S2)
    P1->REN |= (BIT1 + BIT4); // P1.1 and P1.4 enable pull-up/pull-down resistor (S1, S2)
    P1->OUT |= (BIT1 + BIT4); // P1.1 and P1.4 select pull-up resistor (S1, S2)

    while (1)                               // continuous loop
    {
        if (P1->IN & BIT1) {
            P1->OUT &= ~BIT0; // P1.0 (RED LED 1) set low
        }
        else {
            P1->OUT ^= BIT0; // P1.0 (RED LED 1) toggle
        }

        if (P1->IN & BIT4) {
            P1->OUT &= ~BIT2; // P1.2 (BLUE  LED 1) set low
        }
        else {
            P1->OUT ^= BIT2; // P1.4 (BLUE LED 1) toggle
        }

        __delay_cycles(1500000);             // Delay 0.5 seconds
    }
}

