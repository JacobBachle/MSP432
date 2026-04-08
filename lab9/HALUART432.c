//*****************************************************************************
//
//  HAL interface file to Back Channel UART on the MSP432P401r
//
//****************************************************************************

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* UART Configuration Parameter. These are the configuration parameters to
 * make the eUSCI A UART module to operate with a 115200 baud rate. These
 * values were calculated using the online calculator that TI provides
 * at:
 *http://processors.wiki.ti.com/index.php/USCI_UART_Baud_Rate_Gen_Mode_Selection
 */

const eUSCI_UART_ConfigV1 uartConfig =
{
        EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
        26,                                      // BRDIV = 26
        0,                                       // UCxBRF = 0
        0,                                       // UCxBRS = 0
        EUSCI_A_UART_NO_PARITY,                  // No Parity
        EUSCI_A_UART_LSB_FIRST,                  // MSB First
        EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
        EUSCI_A_UART_MODE,                       // UART mode
        EUSCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION,  // Low Frequency Mode
        EUSCI_A_UART_8_BIT_LEN                  // 8 bit data length
};


//  Initialize the BC UART on the MSP432P4

void UART_init(){

    MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_3);
    MAP_CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
    MAP_CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
    MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );

    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
        GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);

    MAP_UART_enableModule(EUSCI_A0_BASE);

}

// Transmit a string from the buffer to BCUART

void UART_transmitString(char *buffer)
{
       while(*buffer != 0){
         MAP_UART_transmitData(EUSCI_A0_BASE, *buffer);
         buffer++;
       }
}
