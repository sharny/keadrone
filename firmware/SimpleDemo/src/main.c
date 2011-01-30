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
#include "LPC17xx.h"
#include "lpc_types.h"
#include "lpc17xx_clkpwr.h"

/* I2STAT register meanings */
typedef enum
{
	//Common
	Mx_START = 0x08,
	Mx_REPEATED_START = 0x10,
	Mx_ARB_LOST = 0x38,
	//TRANSMIT
	MT_ACK_SLAVEADDR = 0x18,
	MT_NACK_SLAVEADDR = 0x20,
	MT_ACK_DATA = 0x28,
	MT_NACK_DATA = 0x30,
	// RECEIVE STATES
	MR_ACK_SLAVEADDR = 0x40,
	MR_NACK_SLAVEADDR = 0x48,
	MR_ACK_DATA = 0x50,
	MR_NACK_DATA = 0x58,
} STATES_MASTER;

LPC_I2C_TypeDef *LPC_I2Cx;
#define STARTBIT	1<<5
#define STOPBIT		1<<4
#define SER_INT		1<<3
#define AA_BIT		1<<2

void i2cStop(void)
{
	// stop bit, since we received NACK instead of ACK
	LPC_I2Cx->I2CONSET = STOPBIT;
	LPC_I2Cx->I2CONCLR = SER_INT | STARTBIT;
}

void i2cStart(void)
{
	// start data request
	LPC_I2Cx->I2CONSET = STARTBIT; // start bit I2C

	int reg;
	while (!(reg & (SER_INT)))
		reg = LPC_I2Cx->I2CONSET;

	// block for capturing the start bit
	while (Mx_START != LPC_I2Cx->I2STAT && Mx_REPEATED_START
			!= LPC_I2Cx->I2STAT)
		;
}

/* Send after start bit, not repeated start */
Bool i2cSendAddress(uint8_t address)
{
	LPC_I2Cx->I2DAT = address;
	LPC_I2Cx->I2CONCLR = SER_INT | STARTBIT | STOPBIT;

	int reg;
	while (!(reg & (SER_INT)))
		reg = LPC_I2Cx->I2CONSET;

	// block for acknowledge
	switch (LPC_I2Cx->I2STAT)
	{
	case MT_NACK_SLAVEADDR:
	case Mx_ARB_LOST:
	case MR_NACK_SLAVEADDR:
		//case MR_ARB_LOST:
		i2cStop();
		return FALSE;
		break;
	case MT_ACK_SLAVEADDR:
	case MR_ACK_SLAVEADDR:
		return TRUE;
	}

	return TRUE;
}

Bool i2cSendData(uint8_t data)
{
	LPC_I2Cx->I2DAT = data;
	// clear the start bit
	LPC_I2Cx->I2CONCLR = SER_INT | STARTBIT | STOPBIT;

	int reg;
	while (!(reg & (SER_INT)))
		reg = LPC_I2Cx->I2CONSET;

	// block for acknowledge
	switch (LPC_I2Cx->I2STAT)
	{
	case MT_NACK_DATA:
	case Mx_ARB_LOST:
		i2cStop();
		return FALSE;
		break;
	case MT_ACK_DATA:
		return TRUE;
	}
}
void i2cStartRepeat(void)
{
	// (repeated) start data request
	LPC_I2Cx->I2CONSET = STARTBIT;
	LPC_I2Cx->I2CONCLR = SER_INT | STOPBIT;

	int reg;
	while (!(reg & (SER_INT)))
		reg = LPC_I2Cx->I2CONSET;

	while (Mx_START != LPC_I2Cx->I2STAT && Mx_REPEATED_START
			!= LPC_I2Cx->I2STAT)
		;
}

Bool i2cRead(uint8_t *data)
{
	LPC_I2Cx->I2CONCLR = SER_INT | STOPBIT | STARTBIT;

	int reg;
	while (!(reg & (SER_INT)))
		reg = LPC_I2Cx->I2CONSET;

	// todo: time-out
	while (LPC_I2Cx->I2STAT != MR_NACK_DATA)
		if (LPC_I2Cx->I2STAT != MR_ACK_DATA)
			return FALSE;

	*data = LPC_I2Cx->I2DATA_BUFFER;
	return TRUE;
}

void i2ctest(void)
{

#define ITG3200_W 0xD0
#define ITG3200_R 0xD1
	Bool eStatus;
	uint16_t retries = 0;
	do
	{
		i2cStart();
		eStatus = i2cSendAddress(ITG3200_W);

		if (retries++ == 3)
			return;
		// automatic retry if no ACK from slave
	} while (eStatus == FALSE);

	// PART 2
	if (i2cSendData(0) == FALSE)
	{
		i2cStop();
		return;
	}

	// PART 3
	i2cStartRepeat();
	if (i2cSendAddress(ITG3200_R) == FALSE)
	{
		i2cStop();
		return;
	}

	// part 4
	uint8_t rxData;
	if (i2cRead(&rxData) == FALSE)
	{
		i2cStop();
		return;
	}

	i2cStop();

}

void i2cInit(void)
{
	//CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCI2C2, ENABLE);

	/* 00 PCLK_peripheral = CCLK/4
	 * 01 PCLK_peripheral = CCLK
	 * 10 PCLK_peripheral = CCLK/2
	 * 11 PCLK_peripheral = CCLK/8
	 */
	//uint16_t value = 0b01;
	//LPC_SC->PCLKSEL1 |= value << 20; // I2C2

	LPC_PINCON->PINMODE0 |= 0x2 << 20; // pin has neither pull-up nor pull-down.
	LPC_PINCON->PINMODE0 |= 0x2 << 22; // pin has neither pull-up nor pull-down.

	// ports used for I2C2 = p0.10 / SDA2 & p0.11 / SCL2
	//	LPC_PINCON->PINMODE_OD0 |= 1 << 10; // make them open drain
	//	LPC_PINCON->PINMODE_OD0 |= 1 << 11;// make them open drain

	//	LPC_PINCON->PINSEL0 |= 2 << 20; // select I2C function
	//	LPC_PINCON->PINSEL0 |= 2 << 22; // select I2C function

	// I2C1 setup (since 2 is not available??)
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCI2C1, ENABLE);

	/* 00 PCLK_peripheral = CCLK/4
	 * 01 PCLK_peripheral = CCLK
	 * 10 PCLK_peripheral = CCLK/2
	 * 11 PCLK_peripheral = CCLK/8
	 */
	uint16_t value = 0b01;
	LPC_SC->PCLKSEL0 |= value << 20; // I2C1

	// ports used for I2C1 = p0.0 / SDA1 & p0.1 / SCL1
	LPC_PINCON->PINMODE_OD0 |= 1 << 0; // make them open drain
	LPC_PINCON->PINMODE_OD0 |= 1 << 1;// make them open drain

	LPC_PINCON->PINSEL0 |= 3 << 0; // select I2C function
	LPC_PINCON->PINSEL0 |= 3 << 2; // select I2C function
	LPC_PINCON->PINMODE0 |= 0x2 << 0; // pin has neither pull-up nor pull-down.
	LPC_PINCON->PINMODE0 |= 0x2 << 2; // pin has neither pull-up nor pull-down.

	LPC_I2Cx = LPC_I2C1;

	LPC_I2Cx->I2SCLH = 400; // see p448
	LPC_I2Cx->I2SCLL = 400;

	LPC_I2Cx->I2CONSET = 1 << 6; // enable I2C

	while (1)
	{
		i2ctest();
	}
}

int main(void)
{
	/* Create the queue. */
	xQueue = xQueueCreate(mainQUEUE_LENGTH, sizeof(unsigned long));

	if (xQueue != NULL)
	{
		/* Start the two tasks as described in the accompanying application
		 note. */
		xTaskCreate( mainTask, ( signed char * ) "Rx", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL );
		xTaskCreate( idleTask, ( signed char * ) "TX", (200), NULL, tskIDLE_PRIORITY, NULL );

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
#define BUFSIZE 10
extern volatile uint32_t I2CCount;
extern volatile uint8_t I2CMasterBuffer[BUFSIZE];
extern volatile uint32_t I2CCmd, I2CMasterState;
extern volatile uint32_t I2CReadLength, I2CWriteLength;

static void mainTask(void *pvParameters)
{
	portTickType xNextWakeTime;
	const unsigned long ulValueToSend = 100UL;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	const char* welcomeMsg = "UART3 Online:\r\n";
	UARTInit(3, 115200); /* baud rate setting */
	UARTSend(3, (uint8_t *) welcomeMsg, strlen(welcomeMsg));
	char buffer[10];
	int value = 0;

	//i2cInit();
	I2CInit();
	for (;;)
	{
		I2CWriteLength = 2;
		I2CReadLength = 2;
		I2CMasterBuffer[0] = ITG3200_W;
		I2CMasterBuffer[1] =  0;	//address
		I2CEngine();
		vTaskDelay(1500);
		//main_spi();
	}
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

		/* Obtain the current P0 state. */
		ulLEDState = LPC_GPIO1->FIOPIN;

		/* Turn the LED off if it was on, and on if it was off. */
		LPC_GPIO1->FIOCLR = ulLEDState & (1 << 1);
		LPC_GPIO1->FIOSET = ((~ulLEDState) & (1 << 1));
		vTaskDelay(500);
	}
}
/*-----------------------------------------------------------*/

