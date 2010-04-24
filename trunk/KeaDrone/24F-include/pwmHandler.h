/*
 * pwmHandler.h
 *
 *  Created on: 19 feb 2010
 *  Author	: W.N. Pietersz
 *	Website	: www.wnpd.nl
 */

#ifndef PWMHANDLER_H_
#define PWMHANDLER_H_
#include "GenericTypeDefs.h"
#include "Compiler.h"

/* Share the same enum (keypad lights equals keypadInputs)*/
#include "inputHandler.h"
typedef INPUT_EV_ID KEYP_ID;

void kpHighlight(KEYP_ID keyNo, BOOL setON);
/* Give a value between 0 and 100 where 100 is max */
void pwmSetLevel(UINT8 dimValueInput);
void pwmInit(void);
void pwmEnableOutputs(BOOL setON);
void pollPwm(void);
void pwmChangeLevel(void);

/*Turn all leds off */
#define pwmLightsOff(void){	\
		TRISE |= 0x3F & ~kpLightOverrideOn;\
		_RD11 = 1;\
	}
/* Turn on the bar-LEDs */
#define pwmLightsOn1(void){	\
		TRISE &= ~(0x3F & kpLightOverrideOff) ;\
	}
/* Turn on the keypad LEDs */
#define pwmLightsOn2(void){	\
		_RD11 = 0;\
	}

#endif /* PWMHANDLER_H_ */
