/*
 * RS232-interfase.c
 *
 *  Created on: 27 apr 2010
 *  Author	: W.N. Pietersz
 *	Website	: www.wnpd.nl
 */
#include "GenericTypeDefs.h"
#include "Compiler.h"

typedef struct
{

} bufferUart;
#define UART_BUFFER_SIZE 20

typedef struct
{
	/* struct for a circular FIFO buffer UART*/
	INT16 bufIn; /* head of the buffer*/
	INT16 bufOut; /* tail of the buffer*/
	INT16 bufSize;
	// start transmission with ':' and end it with 'CR'
	UINT8 functionName[5][UART_BUFFER_SIZE]; // ASCII  e.g. UP
	UINT8 data[10][UART_BUFFER_SIZE]; // e.g. ASCII 100
	// String e.g. :UP,100<CR>:DWN,10<CR> etc.
} UART_BUFFER_QUEUE_TYPE;

UART_BUFFER_QUEUE_TYPE ptrBuffers;

void __attribute__((__interrupt__, auto_psv)) _U1TXInterrupt(void)
{
	IFS0bits.U1TXIF = 0; // clear TX interrupt flag

}

void __attribute__((__interrupt__, auto_psv)) _U1RXInterrupt(void)
{
	_U1RXIF = 0;
}

static BOOL uartGet(UINT8 *currBufferId)
{
	BOOL eStatus = FALSE;

	if (EventBuf.evSize != 0)
	{
		// send the current data to be send (fifo)
		*currBufferId = EventBuf.evOut;

		// mark that we have new data
		return TRUE;
	}
	return FALSE;

	return eStatus;
}

// called after finished with uartGet
static void uartGetDone(void)
{
	// de-credement the buffer size since we used one
			EventBuf.evSize--;
	/* point the tail of buffer to next element*/
	if (EventBuf.evOut >= EV_BUFFER_SIZE)
		EventBuf.evOut = 0;
	else
		EventBuf.evOut++;
}

//EOF required in pointer GyroX,100
void uartPut(UINT8* stringArray)
{
	ptrBuffers[head].data[4] = 1;

	/* check that buffer is not full*/
	if (ptrBuffers.bufSize <= EV_BUFFER_SIZE)
	{
		UINT8 index;

		// Todo: Could check->raise/do an error/exit if there was no valid data?
		for (index = 0; index < 5; index++)
		{
			if (*(stringArray + index) == ',')
				break;
			ptrBuffers.functionName[index][ptrBuffers.bufIn] = x;
		}

		for (index = 0; index < 10; index++)
		{
			if (*(stringArray + index) == '\n')
				break;
			ptrBuffers.data[index][ptrBuffers.bufIn] = y;
		}

		// de-credement the buffer size since we used one
		ptrBuffers.evSize++;
		/* point the head of buffer to next element*/
		if (ptrBuffers.bufIn >= UART_BUFFER_SIZE)
			ptrBuffers.bufIn = 0;
		else
			ptrBuffers.bufIn++;
		return TRUE;
	}
	return FALSE;
}

void serialPortInit(void)
{
#define FCY 40000000
#define BAUDRATE 19200
#define BRGVAL ((FCY/BAUDRATE)/16)-1

	// Zigbee @19k2, No Flow, 8Data, NoParity, 1stopbits
	// RP5 = RX // RP6 = TX

	// Map RX/TX pins
	_RP5R = 3;
	RPINR18 = 5;

	U1MODEbits.STSEL = 0; // 1-stop bit
	U1MODEbits.PDSEL = 0; // No Parity, 8-data bits
	U1MODEbits.ABAUD = 0; // Auto-Baud Disabled
	U1MODEbits.BRGH = 0; // Low Speed mode
	U1BRG = BRGVAL; // BAUD Rate Setting for xxxx
	/* Interrupt generated when a character is transferred
	 * to the Transmit Shift register and the transmit buffer becomes empty*/
	U1STAbits.UTXISEL0 = 0;
	U1STAbits.UTXISEL1 = 1;
	IEC0bits.U1TXIE = 1; // Enable UART TX interrupt
	U1MODEbits.UARTEN = 1; // Enable UART
	U1STAbits.UTXEN = 1; // Enable UART TX

	U1TXREG = '$'; // Transmit one character
	U1TXREG = 'P'; // Transmit one character
	U1TXREG = 'W'; // Transmit one character
	U1TXREG = 'R'; // Transmit one character
}
