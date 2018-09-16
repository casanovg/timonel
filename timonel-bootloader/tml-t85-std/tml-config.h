/*
 *  File: tml-config-h
 *	Project: Timonel - I2C Bootloader for ATtiny85 MCUs
 *	Author: Gustavo Casanova
 *	...........................................
 *	2018-09-16 gustavo.casanova@nicebots.com
 */

#ifndef _TML_CONFIG_H_
#define _TML_CONFIG_H_

// Definitions
#define PAGE_SIZE		64		/* SPM Flash memory page size */
#define RESET_PAGE		0		/* Interrupt vector table address start location */
#define MAXBUFFERTXLN	5		/* Maximum page buffer TX size */

#ifndef __AVR_ATtiny85__
	#define __AVR_ATtiny85__
	#pragma message "   >>>   Run, Timonel, run!   <<<   "
#endif

#define CMD_READPAGE	false	/* This is used mostly for debugging, it takes ~126 bytes of memory. */
								/* Change TIMONEL_START in Makefile.inc to 1900 or lower to compile. */
								
#define CMD_STPGADDR	false	/* If this is disabled, applications can only be flashed starting */
								/* from page 0. This is OK for most standard applications.        */
								
#ifndef F_CPU
	#define F_CPU 8000000UL		/* Default CPU speed for delay.h */
#endif

#define LED_UI_PIN		PB1		/* >>> Use PB1 to monitor activity. <<< */
#define LED_UI_DDR		DDRB	/* >>> WARNING! This is not for use <<< */
#define LED_UI_PORT		PORTB	/* >>> in production!               <<< */
#define TOGGLETIME		0xFFFF	/* LED toggle delay before initialization */
#define I2CDLYTIME		0x7FFF	/* Main loop times to allow the I2C responses to finish */
#define RXDATASIZE		8		/* RX data size for WRITBUFF command */
#define CYCLESTOEXIT	15		/* Main loop cycles before exit to app if not initialized */

#define SR_INIT_1		0		/* Status Bit 1 (1)  : Initialized 1 */
#define SR_INIT_2		1		/* Status Bit 2 (2)  : Initialized 2 */
#define SR_DEL_FLASH	2		/* Status Bit 3 (4)  : Delete flash  */
#define SR_APP_READY	3		/* Status Bit 4 (8)  : Application flased OK, ready to run */
#define SR_EXIT_TML		4		/* Status Bit 5 (16) : Exit Timonel & Run Application */
#define SR_BIT_6		5		/* Status Bit 6 (32) : Not used */
#define SR_BIT_7		6		/* Status Bit 7 (64) : Not used */
#define SR_BIT_8		7		/* Status Bit 8 (128): Not used */

#endif	/* Close ifndef _TML_CONFIG_H_ */
