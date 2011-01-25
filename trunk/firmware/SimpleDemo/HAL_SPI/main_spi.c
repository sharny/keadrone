#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include "lpc17xx_ssp.h"
#include "bma180.h"

#define SSP_CHANNEL LPC_SSP0
#define PORT_CS	LPC_GPIO0
#define PIN_MASK_CS (1<<16)

static const uint8_t segmentLUT[10] = {
//FCPBAGED
		(uint8_t) ~0b11011011, // 0
		(uint8_t) ~0b01010000, // 1
		(uint8_t) ~0b00011111, // 2
		(uint8_t) ~0b01011101, // 3
		(uint8_t) ~0b11010100, // 4
		(uint8_t) ~0b11001101, // 5
		(uint8_t) ~0b11001111, // 6
		(uint8_t) ~0b01011000, // 7
		(uint8_t) ~0b11011111, // 8
		(uint8_t) ~0b11011101, // 9
		};

#ifdef todo
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
#endif

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
		PORT_CS->FIODIR |= 1 << 16; //P0.16 as CSn (acc. meter)

		SSP_ConfigStructInit(&sspChannelConfig);
		SSP_Init(SSP_CHANNEL, &sspChannelConfig);
		SSP_Cmd(SSP_CHANNEL, ENABLE);
	}
	//while (1) {
	if (timeKeeper++ % 500000 == 0) {
		PORT_CS->FIOCLR |= PIN_MASK_CS; //CS low

		sspDataConfig.length = 1;
		sspDataConfig.tx_data = txBuff;
		sspDataConfig.rx_data = rxBuff;

		txBuff[0] = segmentLUT[i++]; //Buffer next numeral

		//Only display 0-9
		if (i == 10)
			i = 0;

		//Transfer to 7-Segment Display Driver
		SSP_ReadWrite(SSP_CHANNEL, &sspDataConfig, SSP_TRANSFER_POLLING);

		PORT_CS->FIOSET |= PIN_MASK_CS; //CS High
	}
	//}
	return 0;
}
