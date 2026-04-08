//*****************************************************************************
//
//  HAL interface file to Back Channel UART on the MSP432P401r
//
//****************************************************************************

#ifndef _UART_H
#define _UART_H

#ifdef __cplusplus
extern "C"
{
#endif

void UART_transmitString(char *buffer);
void UART_init();

#ifdef __cplusplus
}
#endif

#endif
