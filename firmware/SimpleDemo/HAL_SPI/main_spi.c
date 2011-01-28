#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include "lpc17xx_ssp.h"
#include "bma180.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "uart.h"

#define SSP_CHANNEL LPC_SSP0
#define PORT_CS	LPC_GPIO0
#define PIN_MASK_CS (1<<16)

/* The rate at which data is sent to the queue, specified in milliseconds. */
#define delay_1ms			( 1 / portTICK_RATE_MS )

static void delay_ms(uint16_t value) {
	portTickType xNextWakeTime;
	vTaskDelayUntil(&xNextWakeTime, (delay_1ms * value));

}

char read(uint8_t address) {
	//returns the contents of any 1 byte register from any address
	//sets the MSB for every address byte (READ mode)

	char rxBuff;
	SSP_DATA_SETUP_Type sspDataConfig;

	PORT_CS->FIOCLR |= PIN_MASK_CS; //CS low

	address |= 0x80;
	sspDataConfig.length = 1;
	sspDataConfig.tx_data = &address;
	sspDataConfig.rx_data = &rxBuff;

	SSP_ReadWrite(SSP_CHANNEL, &sspDataConfig, SSP_TRANSFER_POLLING);
	address = 0x55;//dummy write
	SSP_ReadWrite(SSP_CHANNEL, &sspDataConfig, SSP_TRANSFER_POLLING);

	PORT_CS->FIOSET |= PIN_MASK_CS; //CS High

	return rxBuff;
}

void write(uint8_t address, char data) {
	//write any data byte to any single address
	//adds a 0 to the MSB of the address byte (WRITE mode)

	address &= 0x7F;
	char rxBuff;
	SSP_DATA_SETUP_Type sspDataConfig;

	printf("\nWriting 0x%x to 0x%x\n", data, address);

	PORT_CS->FIOCLR |= PIN_MASK_CS; //CS low

	delay_ms(1);

	sspDataConfig.tx_data = &address;
	sspDataConfig.rx_data = &rxBuff;
	SSP_ReadWrite(SSP_CHANNEL, &sspDataConfig, SSP_TRANSFER_POLLING);

	delay_ms(1);

	sspDataConfig.tx_data = &data;
	sspDataConfig.rx_data = &rxBuff;
	SSP_ReadWrite(SSP_CHANNEL, &sspDataConfig, SSP_TRANSFER_POLLING);

	delay_ms(1);

	PORT_CS->FIOSET |= PIN_MASK_CS; //CS High

}

// init_BMA180
// Input: range is a 3-bit value between 0x00 and 0x06 will set the range as described in the BMA180 datasheet (pg. 27)
// bw is a 4-bit value between 0x00 and 0x09.  Again described on pg. 27
// Output: -1 on error, 0 on success
int init_BMA180(uint8_t range, uint8_t bw) {
	char temp, temp1;

	// if connected correctly, ID register should be 3
	if (read(ID) != 3)
		return -1;

	//-------------------------------------------------------------------------------------
	// Set ee_w bit
	temp = read(CTRLREG0);
	temp |= 0x10;
	write(CTRLREG0, temp); // Have to set ee_w to write any other registers
	//-------------------------------------------------------------------------------------
	// Set BW
	temp = read(BWTCS);
	temp1 = bw;
	temp1 = temp1 << 4;
	temp &= (~BWMASK);
	temp |= temp1;
	write(BWTCS, temp); // Keep tcs<3:0> in BWTCS, but write new BW
	//-------------------------------------------------------------------------------------
	// Set Range
	temp = read(OLSB1);
	temp1 = range;
	temp1 = (temp1 << RANGESHIFT);
	temp &= (~RANGEMASK);
	temp |= temp1;
	write(OLSB1, temp); //Write new range data, keep other bits the same
	//-------------------------------------------------------------------------------------

	return 0;
}

int main_spi(void) {
	static int startup = FALSE;

	volatile int timeKeeper = 0;
	unsigned char i = 0;
	SSP_CFG_Type sspChannelConfig;
	SSP_DATA_SETUP_Type sspDataConfig;

	uint8_t rxBuff[5];
	uint8_t txBuff[5];

	if (startup == FALSE) {
		startup = TRUE;

		LPC_PINCON->PINSEL0 |= 0x2 << 30; //SCK0 p0.15
		LPC_PINCON->PINSEL1 |= 0x2 << 4; //MOSI0 p0.18
		LPC_PINCON->PINSEL1 |= 0x2 << 2; //MISO0 p0.17

		LPC_PINCON->PINMODE1 |= 0x3 << 4;// pull down mosi pin.
		LPC_PINCON->PINMODE1 |= 0x2 << 2;// pull down mosi pin.

		PORT_CS->FIODIR |= 1 << 16; //P0.16 as CSn (acc. meter)

		SSP_ConfigStructInit(&sspChannelConfig);
		SSP_Init(SSP_CHANNEL, &sspChannelConfig);
		SSP_Cmd(SSP_CHANNEL, ENABLE);
		while (init_BMA180(0x02, 7) != 0) {
			//printf("Error connecting to BMA180\n");
			//printf("E\n");
			delay_ms(1000);
		}
	}
	char temp = 0;
	signed short temp2;

	static char buffer[10];

	static char lsb;

	while (temp != 1) {
		temp = read(ACCXLSB) & 0x01;
	}
	lsb = temp;

	temp = read(ACCXMSB);
	temp2 = (signed short) temp << 8;
	temp2 |= lsb;//read(ACCXLSB);
	temp2 = temp2 >> 2; // Get rid of two non-value bits in LSB


	if (temp2 & 1 << 14)
	//value is negative
	{
		temp2 = ~temp2 & ~0xE000; // Discard neg/pos identifier
		//printf("-%.4d,", temp2);
		sprintf(buffer, "-%.4d,", temp2);

	} else {
		temp2 = temp2 & ~0xE000;
		sprintf(buffer, "%.4d,", temp2);
	}
	UARTSend(3, (uint8_t *) buffer, strlen(buffer));//UART3Count );

	while (temp != 1) {
		temp = read(ACCYLSB) & 0x01;
	}
	lsb = temp;

	temp = read(ACCYMSB);
	temp2 = temp << 8;
	temp2 |= lsb;//read(ACCYLSB);
	temp2 = temp2 >> 2; // Get rid of two non-value bits in LSB

	if (temp2 & 1 << 14)
	//value is negative
	{
		temp2 = ~temp2 & ~0xE000; // discart neg/pos identifier
		//printf("-%.4d,", temp2);
		sprintf(buffer, "-%.4d,", temp2);

	} else {
		temp2 = temp2 & ~0xE000;
		sprintf(buffer, "%.4d,", temp2);
	}
	UARTSend(3, (uint8_t *) buffer, strlen(buffer));//UART3Count );

	while (temp != 1) {
		temp = read(ACCZLSB) & 0x01;
	}
	lsb = temp;
	temp = read(ACCZMSB);
	temp2 = temp << 8;
	temp2 |= lsb;//read(ACCZLSB);
	temp2 = temp2 >> 2; // Get rid of two non-value bits in LSB

	if (temp2 & 1 << 14)
	//value is negative
	{
		temp2 = ~temp2 & ~0xE000; // discart neg/pos identifier
		//printf("-%.4d,", temp2);
		sprintf(buffer, "-%.4d\n", temp2);

	} else {
		temp2 = temp2 & ~0xE000;
		sprintf(buffer, "%.4d\n", temp2);
	}
	UARTSend(3, (uint8_t *) buffer, strlen(buffer));//UART3Count );

	vTaskDelay(1000);
#ifdef not_used
	//while (1) {
	if (timeKeeper++ % 500000 == 0) {
		PORT_CS->FIOCLR |= PIN_MASK_CS; //CS low

		sspDataConfig.length = 1;
		sspDataConfig.tx_data = txBuff;
		sspDataConfig.rx_data = rxBuff;

		//	txBuff[0] = segmentLUT[i++]; //Buffer next numeral

		//Only display 0-9
		if (i == 10)
		i = 0;

		//Transfer to 7-Segment Display Driver
		SSP_ReadWrite(SSP_CHANNEL, &sspDataConfig, SSP_TRANSFER_POLLING);

		PORT_CS->FIOSET |= PIN_MASK_CS; //CS High
	}
#endif
	//}
	return 0;
}
