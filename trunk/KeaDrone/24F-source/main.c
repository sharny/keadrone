#include "GenericTypeDefs.h"
#include "Compiler.h"

/* Include drivers */

#if defined(__PIC24F__)
_CONFIG2(0x79ED)
_CONFIG1(0x3E3F)
#endif

#if defined(__PIC24H__)
//_CONFIG2(0x79ED)
//_CONFIG1(0x3E3F)

_FBS (BWRP_WRPROTECT_OFF)
_FGS (GSS_OFF &GCP_OFF &GWRP_OFF)
_FOSCSEL (FNOSC_FRCPLL &IESO_OFF)
_FOSC (FCKSM_CSECMD &IOL1WAY_OFF &OSCIOFNC_ON )
_FWDT (FWDTEN_OFF &WINDIS_OFF &WDTPRE_PR128 &WDTPOST_PS512)
_FPOR (FPWRT_PWR16 &ALTI2C_OFF)
_FICD (JTAGEN_OFF &ICS_PGD2)

#endif

int main(void)
{

	// Led Red
	_TRISA3 = 0;
	// Led blue
	_TRISA2 = 0;
	// Led green
	_TRISA8 = 0;

	_LATA3 = 1;
	_LATA2 = 1;
	_LATA8 = 1;

	// Gyro Self Test == 1
	_TRISA10 = 0;
	// GyroPwr down == 1
	_TRISA7 = 0;
	//GyroHighPassRst ==1
	_TRISC5 = 0;

	_LATC5 = 0;
	_LATA7 = 0;
	_LATA10 = 0;

	/* FRC Osc@7.37
	 * Divided by 2 (pre), multiplied by 86, devided by 4 (post) == ~79MHz	 */
	_PLLDIV = 86;
	/* Fcy divider; 1 == Fosc/2*/
	_DOZE = 1;

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

	UINT16 adcChannel = 12;
	UINT16 vRef = 0;
	UINT16 ADCValue = 0;
	UINT16 shadow;
	UINT32 simpleCounter = 0;
	_LATA3 = 1;
	for (;;)
	{

		_CH0SB = adcChannel; //CH0SB<4:0>: Channel 0 Positive Input Select for Sample B bits
		_CH0SA = adcChannel; //CH0SA<4:0>: Channel 0 Positive Input Select for Sample A bits
		_VCFG = vRef; //External VREF+ only

		if (abs(((INT32) (ADCValue - shadow))) > 5)
		{
			_LATA3 = 1;
			simpleCounter = 0;
			shadow = ADCValue;
			simpleCounter = 0;
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
