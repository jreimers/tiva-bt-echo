/*
 * This program for the TM4C123GH6PM interfaces with a PAN1321 bluetooth module.
 * It accepts an incoming connection and enters streaming mode. Incoming data is
 * echoed to both the uC debug console (UART0) as well as back to the bluetooth
 * module (UART1). Hardware flow control on the TM4C123GH6PM is utilized to
 * automatically handle RTS and CTS signalling for the H4 UART protocol used by
 * the PAN1321.
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"


//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//
// Setup UARTs
//
void
ConfigureUARTs(void)
{
	// Enable the GPIO Peripheral used by the UARTs.
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	// Enable GPIO pins for UART1 HW flow control
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);

	// Enable UART0
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	// Enable UART1
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

	// Configure GPIO pins for UART0
	ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
	ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
	ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	// Configure GPIO pins for UART1
	ROM_GPIOPinConfigure(GPIO_PB0_U1RX);
	ROM_GPIOPinConfigure(GPIO_PB1_U1TX);
	ROM_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	// Configure GPIO pins for UART1 HW flow control
	ROM_GPIOPinConfigure(GPIO_PC4_U1RTS);
	ROM_GPIOPinConfigure(GPIO_PC5_U1CTS);
	ROM_GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);

	// Enable debug console on UART0 at 115200 baud
	UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
	UARTStdioConfig(0, 115200, 16000000);
	// Setup UART1 at 9600 baud
	UARTClockSourceSet(UART1_BASE, UART_CLOCK_PIOSC);
	UARTConfigSetExpClk(UART1_BASE, 16000000, 9600, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	// Enable RTS/CTS HW flow control for UART1
	UARTFlowControlSet(UART1_BASE, UART_FLOWCONTROL_RX | UART_FLOWCONTROL_TX);
	// Enable UART1, this call also enables the FIFO buffer necessary for HW flow control
	UARTEnable(UART1_BASE);
}
//
// Prints a string (buf) followed by a CRLF (\r\n)
//
void
UART1Println(const char *buf)
{
	while(*buf)
	{
		UARTCharPut(UART1_BASE, *buf++);
	}
	UARTCharPut(UART1_BASE, '\r');
	UARTCharPut(UART1_BASE, '\n');
}

int
main(void)
{
	// Set the clocking to run directly from the crystal.
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
	// Initialize the UARTs
	ConfigureUARTs();

	UARTprintf("Enabling bluetooth\n");

	UART1Println("AT+JRES"); // software reset
	UART1Println("AT+JSEC=1,2,2,04,1234"); // enable security mode 1 with fixed pin '1234'
	UART1Println("AT+JSLN=09,dashboard"); // set device friendly name to 'dashboard'
	UART1Println("AT+JDIS=3"); // make device discoverable

	UARTprintf("Enabled service discovery\n");

	UART1Println("AT+JRLS=1101,11,Serial port,01,000000"); // register local service with serial port profile
	UART1Println("AT+JAAC=1"); // enable auto accepting connection requests

	UARTprintf("Auto accepting connection requests...\n");

	// wait for a device to connect and then enter streaming mode
	while(UARTCharGet(UART1_BASE) != '+') { }
	while(UARTCharGet(UART1_BASE) != 'R') { }
	while(UARTCharGet(UART1_BASE) != 'C') { }
	while(UARTCharGet(UART1_BASE) != 'C') { }

	UARTprintf("Received connection request\n");
	UARTprintf("Entering streaming mode...\n");

	UART1Println("AT+JSCR"); // request streaming mode

	// wait for OK
	while(UARTCharGet(UART1_BASE) != 'O') { }
	while(UARTCharGet(UART1_BASE) != 'K') { }

	UARTprintf("Entered streaming mode\n\n");

	// loop and echo back incoming characters to the bluetooth serial as well as the debug console
	while(1) {
		char c = UARTCharGet(UART1_BASE);
		UARTprintf("%c", c);
		UARTCharPut(UART1_BASE, c);
	}

}
