/*
 * main.c
 *
 *  Created on: 17 feb 2010
 *  Author	: W.N. Pietersz
 *	Website	: www.wnpd.nl
 *	:
 *	:
 */

#include "GenericTypeDefs.h"
#include "Compiler.h"

/* Include drivers */

#if defined(__PIC24H__)
//PIC24HJ64GP204-I/ML
//http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en534555
/* Fuse settings */
_FBS (BWRP_WRPROTECT_OFF)
_FGS (GSS_OFF &GCP_OFF &GWRP_OFF)
_FOSCSEL (FNOSC_FRCPLL &IESO_OFF)
_FOSC (FCKSM_CSECMD &IOL1WAY_OFF &OSCIOFNC_ON )
_FWDT (FWDTEN_OFF &WINDIS_OFF &WDTPRE_PR128 &WDTPOST_PS512)
_FPOR (FPWRT_PWR16 &ALTI2C_OFF)
_FICD (JTAGEN_OFF &ICS_PGD2)

#endif

void gyro1Init(void)
{
#define GYRO1_IO_SELF_TST	_LATC5
#define GYRO1_IO_PWR_DWN	_LATA7
#define GYRO1_IO_RST_HP		_LATA10

	GYRO1_IO_SELF_TST = 0;
	GYRO1_IO_PWR_DWN = 0;
	GYRO1_IO_RST_HP = 0;

	// Gyro Self Test == 1
	_TRISA10 = 0;
	// GyroPwr down == 1
	_TRISA7 = 0;
	//GyroHighPassRst ==1
	_TRISC5 = 0;
}

void ledsInit(void)
{
#define LED_R 	_LATA3
#define LED_GR 	_LATA8
#define LED_B	_LATA2

	LED_R = 0;
	LED_GR = 0;
	LED_B = 0;

	// Led Red
	_TRISA3 = 0;
	// Led blue
	_TRISA2 = 0;
	// Led green
	_TRISA8 = 0;

}

void analogInit(void)
{
	// AN12 == GyroTop1 - AN11 == GyroTop2
	// AN9 == GyroBottom1 - AN10 == GyroBottom2

	_SSRC = 7;//111=Internal counter ends sampling and starts conversion (auto-convert)
	_ASAM = 0; //1 = Sampling begins immediately after last conversion. SAMP bit is auto-set
	_VCFG = 1; //External VREF+ only
	_ADRC = 1; //1 = ADC internal RC clock
	_SAMC = 10; // Auto Sample Time bits

	_ADON = 1;

	_CH0SB = 0; //CH0SB<4:0>: Channel 0 Positive Input Select for Sample B bits
	_CH0SA = 0; //CH0SA<4:0>: Channel 0 Positive Input Select for Sample A bits

}

void analogDemo(void)
{
	UINT16 adcChannel = 12;
	UINT16 vRef = 0;
	static UINT16 ADCValue = 0;
	UINT16 shadow;
	UINT32 simpleCounter = 0;

	static UINT8 dender = 10;

	_CH0SB = adcChannel; //CH0SB<4:0>: Channel 0 Positive Input Select for Sample B bits
	_CH0SA = adcChannel; //CH0SA<4:0>: Channel 0 Positive Input Select for Sample A bits
	_VCFG = vRef; //External VREF+ only

	if (abs(((INT32) (ADCValue - shadow))) >= dender)
	{
		_LATA3 = 1;
		simpleCounter = 0;
		shadow = ADCValue;
		simpleCounter = 0;
		U1TXREG = (ADCValue >> 2) & 0xFF; // Transmit one character
		//U1TXREG = (ADCValue  & 0xFF00)>>8; // Transmit one character
		//U1TXREG = (ADCValue)  & 0xFF; // Transmit one character

	}
	else
	{
		if (simpleCounter < 0xFFF)
		{
			simpleCounter++;

		}
		else
			_LATA3 = 0;
	}

	_SAMP = 1; // 1 = ADC sample/hold amplifiers are sampling
	while (_SAMP == TRUE)
		; // block till done
	while (_DONE == FALSE)
		; // block till done
	ADCValue = ADC1BUF0;
}

int main(void)
{
	/* Set the main-oscillator speed */
	// Configure Oscillator to operate the device at 40MHz
	// FOSC= FIN*M/(N1*N2), FCY=FOSC/2
	// FOSC= 8M*40(2*2)=80MHz for 8M input clock
	PLLFBD = 38; // M=40
	CLKDIVbits.PLLPOST = 0; // N1=2
	CLKDIVbits.PLLPRE = 0; // N2=2
	OSCTUN = 22; // Tune FRC oscillator, if FRC is used
	_DOZE = 1;
	while (OSCCONbits.LOCK != 1)
		; // Wait for PLL to lock

	serialPortInit();
	analogInit();
	for (;;)
	{
		analogDemo();
	}
	return 0;
}

/********************* START USER APPLICATION ***********************/

/**
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */
