/*
 * inputHandler.h
 *
 *  Created on: 24 nov 2009
 *  Author	: W.N. Pietersz
 *	Website	: www.wnpd.nl
 */

#ifndef INPUTHANDLER_H_
#define INPUTHANDLER_H_

#include "delayroutine.h"
#include "GenericTypeDefs.h"

typedef enum keypadEvents
{
	KP_EV_SETON_SHORT,
	KP_EV_SETOFF_SHORT,
	KP_EV_SETON_LONG,
	KP_EV_SETOFF_LONG,
	KP_EV_SETON_CONTINOUS,
	KP_EV_SETOFF_CONTINOUS,
	KP_EV_SETON,
	KP_EV_SETOFF
} INPUT_EV_TYPE;

typedef enum
{
	KEYP_USER_4,
	KEYP_USER_5,
	KEYP_USER_6,
	KEYP_USER_3,
	KEYP_USER_2,
	KEYP_USER_1,

	PAAL_1A,
	PAAL_1B,
	PAAL_2C,
	PAAL_2A,
	PAAL_2B,
	PAAL_1C,

	KEYP_CONF_1,
	KEYP_CONF_2,
	KEYP_CONF_3,
	KEYP_CONF_4
} INPUT_EV_ID;

/* Must be called continuous */
void pollScanInputs(void);
void inputInit(void);
/* Retruns true if new event, else false*/
BOOL inputEventGet(INPUT_EV_ID *evId, INPUT_EV_TYPE *evType);
void inputEvClear(void);

#endif /* INPUTHANDLER_H_ */
