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
#include "i2c.h"

#define BUFSIZE 10
extern volatile uint32_t I2CCount;
extern volatile uint8_t I2CMasterBuffer[BUFSIZE];
extern volatile uint32_t I2CCmd, I2CMasterState;
extern volatile uint32_t I2CReadLength, I2CWriteLength;

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
	I2CInit();
#define ITG3200 0xD0
	uint8_t i2cBuffer[8];
	I2C_DATA i2c;

	while (1)
	{
		i2c.address = ITG3200;
		i2c.slaveRegister = 0;// who am i register
		i2c.readData = TRUE;
		i2c.bufLength = 2;
		i2c.buffer = i2cBuffer;
		I2CEngine(&i2c);
	}
#ifdef OLD
#define ITG3200_W 0xD0
#define ITG3200_R 0xD1
	int16_t value;
	/* Configuration... ****************************/
	// write register 22
	I2CWriteLength = 2;
	I2CReadLength = 0;
	I2CMasterBuffer[0] = ITG3200_W;
	I2CMasterBuffer[1] = 22; //address
	I2CMasterBuffer[2] = 0x18; // must be done for proper operation
	I2CEngine();

	// write register 23 (interrupts)
	I2CWriteLength = 2;
	I2CReadLength = 0;
	I2CMasterBuffer[0] = ITG3200_W;
	I2CMasterBuffer[1] = 23; //address
	I2CMasterBuffer[2] = 0x32;
	I2CEngine();

	// write register 62 (power management)
	I2CWriteLength = 2;
	I2CReadLength = 0;
	I2CMasterBuffer[0] = ITG3200_W;
	I2CMasterBuffer[1] = 62; //address
	I2CMasterBuffer[2] = 0x1;
	I2CEngine();

	for (;;)
	{

		I2CWriteLength = 2;
		I2CReadLength = 1;
		I2CMasterBuffer[0] = ITG3200_W;
		I2CMasterBuffer[1] = 29; //address
		I2CMasterBuffer[2] = ITG3200_R;
		I2CEngine();
		value = I2CMasterBuffer[3] << 8;

		I2CWriteLength = 2;
		I2CReadLength = 1;
		I2CMasterBuffer[0] = ITG3200_W;
		I2CMasterBuffer[1] = 30; //address
		I2CMasterBuffer[2] = ITG3200_R;
		I2CEngine();
		value |= I2CMasterBuffer[3];

		if (value & 1 << 15)
		{
			//negative value
			value = ~(value);
			value &= (uint16_t) 0x7FF;
			printf("-%d\n", value);
		}
		else
		{
			printf("%d\n", value);
		}

		//vTaskDelay(1);
#endif
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

