/*
 * flashSaveOperations.c
 *
 *  Created on: 10 mrt 2010
 *  Author	: W.N. Pietersz
 *	Website	: www.wnpd.nl
 */
#include "GenericTypeDefs.h"
#include "Compiler.h"
#include "flashSaveOperations.h"

/* We need the compiler to align one large space (a flash-block) at
 * a specific address location so we can easily do our operations on the variables*/
#define FLASH_SECTION __attribute__ ((aligned (ERASE_BLOCK_SIZE),section(".STORAGE_FILES"),address(FILES_ADDRESS),space(psv)))
/* This is the actual variable */
static ROM UINT16 FLASH_SECTION pgmData[ERASE_BLOCK_SIZE] =
{ 0xE1, 0x2E, 0x92, 0x1E, 0x0 };

/* IN CASE OF MEMORY SAVING: PUT THIS VAR ON THE STACK!
 * We use this buffer to temporally store the PGM data inside since
 * for every write on a already used location we first have to clear the entire row*/
static UINT16 flashBuffer[ERASE_BLOCK_SIZE];

/* note that a block equals 512 * 16byte values
 * Parameter: Start address of the -to be erased- block*/
static void EraseBlock(UINT16 address)
{
	// C example using MPLAB C30
	unsigned long progAddr = address; // Address of row to write
	unsigned int offset;

	//Set up pointer to the first memory location to be written
	TBLPAG = progAddr >> 16; // Initialize PM Page Boundary SFR
	offset = progAddr & 0xFFFF; // Initialize lower word of address
	__builtin_tblwtl(offset, 0xFFFF); // Set base address of erase block
	// with dummy latch write
	NVMCON = 0x4042; // Initialize NVMCON
	asm("DISI #16");
	// Block all interrupts with priority <7
	// for next 5 instructions
	__builtin_write_NVM(); // C30 function to perform unlock
	// sequence and set WR

}
/* LoadWriteBuffer has to be executed first, than this function to actually program*/
static void ProgramSequence(void)
{
	// C example using MPLAB C30
	asm("DISI #5");
	// Block all interrupts with priority < 7
	// for next 5 instructions
	__builtin_write_NVM(); // Perform unlock sequence and set WR
}

/* Before we can actually program the FLASH
 * we first need to load the hardware buffers
 * Note that I used only 16bits of the flash memory instead of the possible 24bits
 * If you want the 24bits you need to change the 0xFFFF to your buffer.
 * "__builtin_tblwth(offset, 0xFFFF); // Write to upper byte"
 * Note that only the low bytes are usable
 *
 * Parameters: start address of the PGM, and pointer to buffer with at least 64 elements
 * */
static void LoadWriteBuffer(UINT16 address, UINT16 *buffer)
{
	// C example using MPLAB C30
#define NUM_INSTRUCTION_PER_ROW 64
	unsigned int offset;
	unsigned int i;
	unsigned long progAddr = address; // Address of row to write
	//	unsigned int progData[2 * NUM_INSTRUCTION_PER_ROW]; // Buffer of data to write
	//Set up NVMCON for row programming
	NVMCON = 0x4001; // Initialize NVMCON
	//Set up pointer to the first memory location to be written
	TBLPAG = progAddr >> 16; // Initialize PM Page Boundary SFR
	offset = progAddr & 0xFFFF; // Initialize lower word of address
	//Perform TBLWT instructions to write necessary number of latches
	for (i = 0; i < NUM_INSTRUCTION_PER_ROW; i++)
	{
		__builtin_tblwtl(offset, buffer[i]); // Write to address low word
		__builtin_tblwth(offset, 0xFFFF); // Write to upper byte
		offset = offset + 2; // Increment address
	}

	ProgramSequence();
}

void flashSave(FLASH_DATA *flashTable)
{
	UINT16 counter;
	BOOL flashNeedsUpdate = FALSE;

	/* Check if the current PGM is different than the new once */
	for (counter = flashTable->startAddress; counter < (flashTable->size
			+ flashTable->startAddress); counter++)
	{
		if (flashTable->buffer[counter] != pgmData[counter])
		{
			/* todo: could check here if the cell is still writable (== 0xFFFF ??)
			 * to prevent wearing out of the flash in case of heavy use
			 */
			flashNeedsUpdate = TRUE;
		}
	}

	if (flashNeedsUpdate == TRUE)
	{
		/* copy the memory block to memory */
		for (counter = 0; counter != 512; counter++)
		{
			flashBuffer[counter] = pgmData[counter];
		}

		/* Copy the new data inside the buffer */
		for (counter = flashTable->startAddress; counter < (flashTable->size
				+ flashTable->startAddress); counter++)
		{
			flashBuffer[counter] = flashTable->buffer[counter];
		}

		EraseBlock(FILES_ADDRESS);

		/* Restore the (changed) buffer to the PGM memory */
		for (counter = 0; counter < 8; counter++)
		{
			LoadWriteBuffer((FILES_ADDRESS + (counter * 64)),
					&flashBuffer[(counter * 64)]);
			ProgramSequence();
		}
	}
}

void flashReturn(FLASH_DATA *flashTable)
{
	UINT16 counter;
	/* Copy the PGM data to the parameter pointer buffer */
	for (counter = flashTable->startAddress; counter < (flashTable->size
			+ flashTable->startAddress); counter++)
	{
		flashTable->buffer[counter] = pgmData[counter];
	}
}
/*
 *
 *
 *
 *
 *
 *
 *
 */
