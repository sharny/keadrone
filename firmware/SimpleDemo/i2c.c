/*****************************************************************************
 *   i2c.c:  I2C C file for NXP LPC17xx Family Microprocessors
 *
 *   Copyright(C) 2009, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2009.05.26  ver 1.00    Prelimnary version, first Release
 *
 *****************************************************************************/
#include "LPC17xx.h"
//#include "type.h"
#include "i2c.h"
#include "lpc17xx_clkpwr.h"

volatile uint32_t I2CMasterState = I2C_IDLE;
volatile uint32_t I2CSlaveState = I2C_IDLE;

volatile uint32_t I2CCmd;
volatile uint32_t I2CMode;

volatile uint8_t I2CMasterBuffer[BUFSIZE];
volatile uint8_t I2CSlaveBuffer[BUFSIZE];
volatile uint32_t I2CCount = 0;
volatile uint32_t I2CReadLength;
volatile uint32_t I2CWriteLength;

volatile uint32_t RdIndex = 0;
volatile uint32_t WrIndex = 0;
LPC_I2C_TypeDef *LPC_I2Cx;

/* 
 From device to device, the I2C communication protocol may vary,
 in the example below, the protocol uses repeated start to read data from or
 write to the device:
 For master read: the sequence is: STA,Addr(W),offset,RE-STA,Addr(r),data...STO
 for master write: the sequence is: STA,Addr(W),length,RE-STA,Addr(w),data...STO
 Thus, in state 8, the address is always WRITE. in state 10, the address could
 be READ or WRITE depending on the I2CCmd.
 */
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
/*****************************************************************************
 ** Function name:		I2C0_IRQHandler
 **
 ** Descriptions:		I2C0 interrupt handler, deal with master mode
 **						only.
 **
 ** parameters:			None
 ** Returned value:		None
 **
 *****************************************************************************/
void I2C1_IRQHandler(void)
{
	STATES_MASTER StatValue;

	/* this handler deals with master read and master write only */
	StatValue = LPC_I2Cx->I2STAT;

	switch (StatValue)
	{
	case Mx_START: /* A Start condition is issued. */
		LPC_I2Cx->I2DAT = I2CMasterBuffer[0];
		LPC_I2Cx->I2CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC);
		I2CMasterState = I2C_STARTED;
		break;

	case Mx_REPEATED_START: /* A repeated started is issued */
		if (I2CCmd == LM75_TEMP)
		{
			LPC_I2Cx->I2DAT = I2CMasterBuffer[2];
		}
		LPC_I2Cx->I2CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC);
		I2CMasterState = I2C_RESTARTED;
		break;

	case MT_ACK_SLAVEADDR: /* Regardless, it's a ACK */
		if (I2CMasterState == I2C_STARTED)
		{
			LPC_I2Cx->I2DAT = I2CMasterBuffer[1 + WrIndex];
			WrIndex++;
			I2CMasterState = DATA_ACK;
		}
		LPC_I2Cx->I2CONCLR = I2CONCLR_SIC;
		break;

	case MT_ACK_DATA: /* Data byte has been transmitted, regardless ACK or NACK */
	case MT_NACK_DATA:
		if (WrIndex != I2CWriteLength)
		{
			LPC_I2Cx->I2DAT = I2CMasterBuffer[1 + WrIndex]; /* this should be the last one */
			WrIndex++;
			if (WrIndex != I2CWriteLength)
			{
				I2CMasterState = DATA_ACK;
			}
			else
			{
				I2CMasterState = DATA_NACK;
				if (I2CReadLength != 0)
				{
					LPC_I2Cx->I2CONSET = I2CONSET_STA; /* Set Repeated-start flag */
					I2CMasterState = I2C_REPEATED_START;
				}
			}
		}
		else
		{
			if (I2CReadLength != 0)
			{
				LPC_I2Cx->I2CONSET = I2CONSET_STA; /* Set Repeated-start flag */
				I2CMasterState = I2C_REPEATED_START;
			}
			else
			{
				I2CMasterState = DATA_NACK;
				LPC_I2Cx->I2CONSET = I2CONSET_STO; /* Set Stop flag */
			}
		}
		LPC_I2Cx->I2CONCLR = I2CONCLR_SIC;
		break;

	case MR_ACK_SLAVEADDR: /* Master Receive, SLA_R has been sent */
		LPC_I2Cx->I2CONSET = I2CONSET_AA; /* assert ACK after data is received */
		LPC_I2Cx->I2CONCLR = I2CONCLR_SIC;
		break;

	case MR_ACK_DATA: /* Data byte has been received, regardless following ACK or NACK */
	case MR_NACK_DATA:
		I2CMasterBuffer[3 + RdIndex] = LPC_I2Cx->I2DAT;
		RdIndex++;
		if (RdIndex != I2CReadLength)
		{
			I2CMasterState = DATA_ACK;
		}
		else
		{
			RdIndex = 0;
			I2CMasterState = DATA_NACK;
		}
		LPC_I2Cx->I2CONSET = I2CONSET_AA; /* assert ACK after data is received */
		LPC_I2Cx->I2CONCLR = I2CONCLR_SIC;
		break;

	case MT_NACK_SLAVEADDR: /* regardless, it's a NACK */

	case MR_NACK_SLAVEADDR:
		LPC_I2Cx->I2CONCLR = I2CONCLR_SIC;
		I2CMasterState = DATA_NACK;
		break;

	case 0x38: /* Arbitration lost, in this example, we don't
	 deal with multiple master situation */
	default:
		LPC_I2Cx->I2CONCLR = I2CONCLR_SIC;
		break;
	}
}

/*****************************************************************************
 ** Function name:		I2CStart
 **
 ** Descriptions:		Create I2C start condition, a timeout
 **				value is set if the I2C never gets started,
 **				and timed out. It's a fatal error.
 **
 ** parameters:			None
 ** Returned value:		true or false, return false if timed out
 **
 *****************************************************************************/
uint32_t I2CStart(void)
{
	uint32_t timeout = 0;
	uint32_t retVal = FALSE;

	/*--- Issue a start condition ---*/
	LPC_I2Cx->I2CONSET = I2CONSET_STA; /* Set Start flag */

	/*--- Wait until START transmitted ---*/
	while (1)
	{
		if (I2CMasterState == I2C_STARTED)
		{
			retVal = TRUE;
			break;
		}
		if (timeout >= MAX_TIMEOUT)
		{
			retVal = FALSE;
			break;
		}
		timeout++;
	}
	return (retVal);
}

/*****************************************************************************
 ** Function name:		I2CStop
 **
 ** Descriptions:		Set the I2C stop condition, if the routine
 **				never exit, it's a fatal bus error.
 **
 ** parameters:			None
 ** Returned value:		true or never return
 **
 *****************************************************************************/
uint32_t I2CStop(void)
{
	LPC_I2Cx->I2CONSET = I2CONSET_STO; /* Set Stop flag */
	LPC_I2Cx->I2CONCLR = I2CONCLR_SIC; /* Clear SI flag */

	/*--- Wait for STOP detected ---*/
	while (LPC_I2Cx->I2CONSET & I2CONSET_STO)
		;
	return TRUE;
}

/*****************************************************************************
 ** Function name:		I2CInit
 **
 ** Descriptions:		Initialize I2C controller
 **
 ** parameters:			I2c mode is either MASTER or SLAVE
 ** Returned value:		true or false, return false if the I2C
 **				interrupt handler was not installed correctly
 **
 *****************************************************************************/
uint32_t I2CInit(uint32_t I2cMode)
{
#ifdef I2C0
	LPC_SC->PCONP |= (1 << 19);

	/* set PIO0.27 and PIO0.28 to I2C0 SDA and SCK */
	/* function to 01 on both SDA and SCK. */
	LPC_PINCON->PINSEL1 &= ~0x03C00000;
	LPC_PINCON->PINSEL1 |= 0x01400000;

#endif

	LPC_PINCON->PINMODE0 |= 0x2 << 22; // pin has neither pull-up nor pull-down.
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
	/*--- Clear flags ---*/
	LPC_I2Cx->I2CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC
			| I2CONCLR_I2ENC;

	/*--- Reset registers ---*/
	LPC_I2Cx->I2SCLL = I2SCLL_SCLL;
	LPC_I2Cx->I2SCLH = I2SCLH_SCLH;
#ifdef OLD_
	if (I2cMode == I2CSLAVE)
	{
		LPC_I2Cx->I2ADR0 = LM75_ADDR;
	}
#endif

	/* Install interrupt handler */
	//todo: choose I2Cx
	NVIC_EnableIRQ(I2C1_IRQn);

	LPC_I2Cx->I2CONSET = I2CONSET_I2EN;
	return (TRUE);
}

/*****************************************************************************
 ** Function name:		I2CEngine
 **
 ** Descriptions:		The routine to complete a I2C transaction
 **				from start to stop. All the intermitten
 **				steps are handled in the interrupt handler.
 **				Before this routine is called, the read
 **				length, write length, I2C master buffer,
 **				and I2C command fields need to be filled.
 **				see i2cmst.c for more details.
 **
 ** parameters:			None
 ** Returned value:		true or false, return false only if the
 **				start condition can never be generated and
 **				timed out.
 **
 *****************************************************************************/
uint32_t I2CEngine(void)
{
	I2CMasterState = I2C_IDLE;
	RdIndex = 0;
	WrIndex = 0;
	if (I2CStart() != TRUE)
	{
		I2CStop();
		return (FALSE);
	}

	while (1)
	{
		if (I2CMasterState == DATA_NACK)
		{
			I2CStop();
			break;
		}
	}
	return (TRUE);
}

/******************************************************************************
 **                            End Of File
 ******************************************************************************/

