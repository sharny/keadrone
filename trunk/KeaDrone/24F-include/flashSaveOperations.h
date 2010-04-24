/*
 * flashSaveOperations.h
 *
 *  Created on: 10 mrt 2010
 *  Author	: W.N. Pietersz
 *	Website	: www.wnpd.nl
 */


#ifndef FLASHSAVEOPERATIONS_H_
#define FLASHSAVEOPERATIONS_H_

#include "GenericTypeDefs.h"
#include "Compiler.h"

/*
 * Note that these defines are different for each PIC24 device since
 * every device has a different memory footprint. I used the PSV
 * (program space visibility) so the acces time to the PGM is much faster*/
#define FILES_ADDRESS 0x7800
/* must be a multiple of 512 */
#define ERASE_BLOCK_SIZE 512

typedef struct
{
	/* please note that this address starts from 0 till 512 and is NOT the PGM address*/
	UINT16 startAddress; // the start address where the buffer should be saved
	UINT8 size; // the size of the buffer array
	UINT16 *buffer; // an array that hold the data to be saved
} FLASH_DATA;

void flashReturn(FLASH_DATA *flashTable);
void flashSave(FLASH_DATA *flashTable);

#endif /* FLASHSAVEOPERATIONS_H_ */
