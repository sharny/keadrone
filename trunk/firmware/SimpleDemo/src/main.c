/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* The rate at which data is sent to the queue, specified in milliseconds. */
#define mainQUEUE_SEND_FREQUENCY_MS			( 100 / portTICK_RATE_MS )

/* The number of items the queue can hold.  This is 1 as the receive task
 will remove items as they are added, meaning the send task should always find
 the queue empty. */
#define mainQUEUE_LENGTH					( 1 )

/*
 * The tasks as described in the accompanying PDF application note.
 */
static void mainTask(void *pvParameters);
static void idleTask(void *pvParameters);

/* The queue used by both tasks. */
static xQueueHandle xQueue = NULL;

/*-----------------------------------------------------------*/

int main(void)
{
	/* Create the queue. */
	xQueue = xQueueCreate(mainQUEUE_LENGTH, sizeof(unsigned long));

	if (xQueue != NULL)
	{
		/* Start the two tasks as described in the accompanying application
		 note. */
		xTaskCreate( mainTask, ( signed char * ) "Rx", (200), NULL, tskIDLE_PRIORITY+1, NULL );
		xTaskCreate( idleTask, ( signed char * ) "TX", (100), NULL, tskIDLE_PRIORITY, NULL );

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
#include "lpc17xx_clkpwr.h"

#define BUFSIZE 10

static void mainTask(void *pvParameters)
{
	portTickType xNextWakeTime;
	const unsigned long ulValueToSend = 100UL;

	/* Initialize xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	UARTInit(3, 115200); /* baud rate setting */
	printf("Maintask: Running\n");

	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPIO, ENABLE);
	spiInit();
	gyroInit();

	while (1)
		vTaskDelay(100);
}

/*-----------------------------------------------------------*/

static void idleTask(void *pvParameters)
{
	uint32_t ulReceivedValue;

	/* Initialize P1_1 for the LED. */
	LPC_PINCON->PINSEL2 &= (~(3 << 2));
	LPC_GPIO1->FIODIR |= (1 << 1);

	for (;;)
	{
		uint32_t ulLEDState;
#ifdef disabled
		/* Obtain the current P0 state. */
		ulLEDState = LPC_GPIO1->FIOPIN;

		/* Turn the LED off if it was on, and on if it was off. */
		LPC_GPIO1->FIOCLR = ulLEDState & (1 << 1);
		LPC_GPIO1->FIOSET = ((~ulLEDState) & (1 << 1));
#endif
		vTaskDelay(500);
	}
}
/*-----------------------------------------------------------*/

