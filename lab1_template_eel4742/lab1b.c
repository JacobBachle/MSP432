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

    P2->DIR |= BIT0;                        // P2.0 set as output (RED LED 2)
    P2->DIR |= BIT1;                        // P2.1 set as output (GREEN LED 2)
    P2->DIR |= BIT2;                        // P2.2 set as output (BLUE LED 2)


    while (1)                               // continuous loop
    {
        P2->OUT |= BIT0;                    // OUT set to high on P2.0 (RED LED 2)
        __delay_cycles(750000);             // Delay 0.25 seconds
        P2->OUT &= ~BIT0;                   // OUT set to low on P2.0 (RED LED 2)
        __delay_cycles(750000);             // Delay 0.25 seconds

        P2->OUT |= BIT1;                    // OUT set to high on P2.1 (GREEN LED 2)
        __delay_cycles(750000);             // Delay 0.25 seconds
        P2->OUT &= ~BIT1;                   // OUT set to low on P2.1 (GREEN LED 2)
        __delay_cycles(750000);             // Delay 0.25 seconds

        P2->OUT |= BIT2;                    // OUT set to high on P2.2 (BLUE LED 2)
        __delay_cycles(750000);             // Delay 0.25 seconds
        P2->OUT &= ~BIT2;                   // OUT set to low on P2.2 (BLUE LED 2)
        __delay_cycles(750000);             // Delay 0.25 seconds

        P1->OUT &= ~BIT0;                   // OUT set to low on P1.0 (RED LED 1)
    }
}

