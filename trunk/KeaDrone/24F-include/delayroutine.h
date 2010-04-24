/*
 * delayroutine.h
 *
 *  Created on: 18 feb 2010
 *  Author	: W.N. Pietersz
 *	Website	: www.wnpd.nl
 */


#ifndef DELAYROUTINE_H_
#define DELAYROUTINE_H_
#include "GenericTypeDefs.h"

void systemtickInit(void);
UINT8 ucDelayMs(INT16 ms, BYTE *ptrId);
void resetDelay(unsigned short ms, unsigned char *taskId);
#endif /* DELAYROUTINE_H_ */
