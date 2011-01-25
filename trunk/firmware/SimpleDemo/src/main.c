/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* UART TEST */
#include "LPC17xx.h"
#include "type.h"
#include "uart.h"
#include <string.h>

extern volatile uint32_t UART3Count;
extern volatile uint8_t UART3Buffer[BUFSIZE];

/* END UART TEST */

/* Priorities at which the tasks are created. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

/* The bit of port 0 that the LPCXpresso LPC13xx LED is connected. */
#define mainLED_BIT 						( 1 )

/* The rate at which data is sent to the queue, specified in milliseconds. */
#define mainQUEUE_SEND_FREQUENCY_MS			( 500 / portTICK_RATE_MS )

/* The number of items the queue can hold.  This is 1 as the receive task
 will remove items as they are added, meaning the send task should always find
 the queue empty. */
#define mainQUEUE_LENGTH					( 1 )

/*
 * The tasks as described in the accompanying PDF application note.
 */
static void prvQueueReceiveTask(void *pvParameters);
static void prvQueueSendTask(void *pvParameters);

/*
 * Simple function to toggle the LED on the LPCXpresso LPC17xx board.
 */
static void prvToggleLED(void);

/* The queue used by both tasks. */
static xQueueHandle xQueue = NULL;

/*-----------------------------------------------------------*/

int main(void) {

	/* Create the queue. */
	xQueue = xQueueCreate(mainQUEUE_LENGTH, sizeof(unsigned long));

	if (xQueue != NULL) {
		/* Start the two tasks as described in the accompanying application
		 note. */
		xTaskCreate( prvQueueReceiveTask, ( signed char * ) "Rx", configMINIMAL_STACK_SIZE, NULL, mainQUEUE_RECEIVE_TASK_PRIORITY, NULL );
		xTaskCreate( prvQueueSendTask, ( signed char * ) "TX", (200), NULL, mainQUEUE_SEND_TASK_PRIORITY, NULL );

		/* Start the tasks running. */
		vTaskStartScheduler();
	}

	/* If all is well we will never reach here as the scheduler will now be
	 running.  If we do reach here then it is likely that there was insufficient
	 heap available for the idle task to be created. */
	for (;;)
		;
}
/*-----------------------------------------------------------*/
// ***********
// * Defines related to ADC
// ***********
// PCADC / PCAD
#define ADC_POWERON (1 << 12)
#define PCLK_ADC 24
#define PCLK_ADC_MASK (3 << 24)

// AD0.0 - P0.23, PINSEL1 [15:14] = 01
#define SELECT_ADC0 (0x1<<14)

// ADOCR constants
#define START_ADC (1<<24)
#define OPERATIONAL_ADC (1 << 21)
#define SEL_AD0 (1 <<0)
#define ADC_DONE_BIT	(1 << 31)
//WP BELOW ADD
#define SEL_AD2 (1 <<2)
#define SEL_AD5 (1 <<5)
#define SEL_AD6 (1 <<6)
#define SEL_AD7 (1 <<7)

// Buffer to contain string version of value read from ADC
// 4 character, plus zero terminator
char strbuf[5];

static void adcVoltage(void) {
	static int startup = 0;
	int adval, adval_64;
	// ************
	// * Set up ADC
	// ************

	if (startup == 0) {
		startup = 1;
		// Turn on power to ADC block
		LPC_SC->PCONP |= ADC_POWERON;

		// Turn on ADC peripheral clock
		LPC_SC->PCLKSEL0 &= ~(PCLK_ADC_MASK);
		LPC_SC->PCLKSEL0 |= (3 << PCLK_ADC);

		// Set P0.23 to AD0.0 in PINSEL1
		//LPC_PINCON->PINSEL1 |= SELECT_ADC0;
		//P0.3 == AD0.6 == TEMP
		LPC_PINCON->PINSEL0 |= (2 << 6);
		LPC_PINCON->PINMODE0 |= (2 << 6);
		// P0.2 -- AD0.7 == maxsonar
		LPC_PINCON->PINSEL0 |= (2 << 4);
		LPC_PINCON->PINMODE0 |= (2 << 4);
		// P0.25 -- AD0.2 == batt. volt
		LPC_PINCON->PINSEL1 |= (1 << 18);
		LPC_PINCON->PINMODE1 |= (1 << 18);
		// P1.31 -- AD0.4 == batt. volt
		LPC_PINCON->PINSEL3 |= (3 << 30);
		LPC_PINCON->PINMODE3 |= (3 << 30);
		//max clock div.
		LPC_ADC->ADCR |= 0xFF << 8;
	}

	// loop below:

	// Start A/D conversion for on AD0.0
	LPC_ADC->ADCR = START_ADC | OPERATIONAL_ADC | SEL_AD6;

	do {
		adval = LPC_ADC->ADGDR; // Read A/D Data Register
	} while ((adval & ADC_DONE_BIT) == 0); // Wait for end of A/D Conversion

	// Stop A/D Conversion
	LPC_ADC->ADCR &= ~(START_ADC | OPERATIONAL_ADC | 0xFF);

	// Extract AD0.X value - 12 bit result in bits [15:4]
	adval = (adval >> 4) & 0x0FFF;

	//Convert integer ADC value to string
	sprintf(strbuf, "%04d", adval);
	printf(strbuf);
	printf("\n");
}
/*-----------------------------------------------------------*/

static void prvQueueSendTask(void *pvParameters) {
	portTickType xNextWakeTime;
	const unsigned long ulValueToSend = 100UL;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	const char* welcomeMsg = "UART3 Online:\r\n";
	UARTInit(3, 9600); /* baud rate setting */
	UARTSend(3, (uint8_t *) welcomeMsg, strlen(welcomeMsg));
	char buffer[10];
	int value = 0;

	for (;;) {
		/* Place this task in the blocked state until it is time to run again.
		 The block state is specified in ticks, the constant used converts ticks
		 to ms.  While in the blocked state this task will not consume any CPU
		 time. */
		vTaskDelayUntil(&xNextWakeTime, mainQUEUE_SEND_FREQUENCY_MS );

		/* Send to the queue - causing the queue receive task to flash its LED.
		 0 is used as the block time so the sending operation will not block -
		 it shouldn't need to block as the queue should always be empty at this
		 point in the code. */
		xQueueSend( xQueue, &ulValueToSend, 0 );

		adcVoltage();

		//		if ( UART3Count != 0 )
		//		{
		//		LPC_UART3->IER = IER_THRE | IER_RLS; /* Disable RBR */
		value++;
		sprintf(buffer, "%d -", value);
		UARTSend(3, (uint8_t *) buffer, strlen(buffer));//UART3Count );
		UARTSend(3, (uint8_t *) welcomeMsg, strlen(welcomeMsg));//UART3Count );
		//UART3Count = 0;
		//		LPC_UART3->IER = IER_THRE | IER_RLS | IER_RBR; /* Re-enable RBR */
		//		}

		main_spi();

	}
}
/*-----------------------------------------------------------*/

static void prvQueueReceiveTask(void *pvParameters) {
	unsigned long ulReceivedValue;

	/* Initialise P1_1 for the LED. */
	LPC_PINCON->PINSEL2 &= (~(3 << 2));
	LPC_GPIO1->FIODIR |= (1 << mainLED_BIT);

	for (;;) {
		/* Wait until something arrives in the queue - this task will block
		 indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
		 FreeRTOSConfig.h. */
		xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

		/*  To get here something must have been received from the queue, but
		 is it the expected value?  If it is, toggle the LED. */
		if (ulReceivedValue == 100UL) {
			prvToggleLED();
		}
	}
}
/*-----------------------------------------------------------*/

static void prvToggleLED(void) {
	unsigned long ulLEDState;

	/* Obtain the current P0 state. */
	ulLEDState = LPC_GPIO1->FIOPIN;

	/* Turn the LED off if it was on, and on if it was off. */
	LPC_GPIO1->FIOCLR = ulLEDState & (1 << mainLED_BIT);
	LPC_GPIO1->FIOSET = ((~ulLEDState) & (1 << mainLED_BIT));
}
/*-----------------------------------------------------------*/

