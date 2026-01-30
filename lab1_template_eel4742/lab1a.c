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
    P2->DIR |= BIT0;                        // P2.0 set as output (RED LED 2)


    while (1)                               // continuous loop
    {
        P1->OUT ^= BIT0;                    // Toggle P1.0 RED LED 1
        P2->OUT ^= BIT0;                    // Toggle P2.0, RED LED 2
        __delay_cycles(1500000);        // Delay 0.5 seconds
    }
}

