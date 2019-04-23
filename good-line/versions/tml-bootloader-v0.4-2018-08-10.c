// ********************************************************
// *  Timonel Bootloader (Interrupt-Free)                 *
// *  ===================================                 *
// *  I2C Slave Bootloader Firmware for AVR uControllers  *
// *  ..................................................  *
// *  Author: Gustavo Casanova  / Nicebots                *
// *  ..................................................  *
// *  Firmware Version: 0.4 | MCU: ATtiny85               *
// *  2018-08-10 gustavo.casanova@nicebots.com            *
// ********************************************************
//
// This version works OK for flashing 1 page (running WriteFlashTest on ESP), but
// it hangs-up after flashing. It has to be restarted before flashing a new page ...
//
// Timonel bootloader I2C commands:
// a - (STDPB1_1) Set ATtiny85 PB1 = 1
// s - (STDPB1_0) Set ATtiny85 PB1 = 0
// v - (GETTMNLV) Show firmware version
// r - (EXITTMNL) Exit bootloader and run application
// e - (DELFLASH) Delete application flash memory
// b - (STPGADDR) Set flash address pointer

// Defines
#ifndef __AVR_ATtiny85__ 
	#define __AVR_ATtiny85__
	//#pragma message ("   >>>   GO NICEBOTS GO!   <<<")
#endif

// Includes
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stddef.h>
#include <stdbool.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include "../../nb-libs/nb-usitwisl-if.h"
#include "../../nb-libs/nb-i2c-cmd.h"

/* Verify that the bootloader start address aligns with page size */
#if BOOTLOADER_ADDRESS % SPM_PAGESIZE != 0
	#error "Timonel BOOTLOADER_ADDRESS in makefile must be a multiple of chip's pagesize"
#endif  

#if SPM_PAGESIZE > 256
	#error "Timonel only supports pagesizes up to 256 bytes"
#endif

#if ((AUTO_EXIT_MS > 0) && (AUTO_EXIT_MS < 1000))
	#error "Do not set AUTO_EXIT_MS to below 1s to allow Timonel to function properly"
#endif

// Type definitions
typedef uint8_t byte;
typedef uint16_t word;
typedef void (*fptr_t)(void);  		// CLAWSON

#ifndef F_CPU
	#define F_CPU 8000000UL			/* Default CPU speed for delay.h */
#endif

#define I2C_ADDR		0x0A		/* I2C Address 10 */
#define TIMONEL_VER_MAJOR 0
#define TIMONEL_VER_MINOR 4
//#define APPL_ADDRESS	0x011E		/* Application start (ZX=0x11E, SOS=0x1E) */
#define APP_JMP_VECTOR	0x1A3E		/* 2-byte jump vector located 1 word (2 bytes) before bootloader */
#define PAGE_SIZE 		64			/* SPM Flash memory page size */
#define INTVECT_PAGE_ADDRESS 0x0	/* Interrupt vector table address start location */
#define BOOT_PAGE_ADDRESS 0x1A40	/* Timonel bootloader start address */

#define LED_PIN			PB1			/* >>> Use PB4 to monitor  <<< */
#define LED_DDR			DDRB		/* >>> activity until ADC2 <<< */
#define LED_PORT		PORTB		/* >>> is implemented      <<< */
#define PWR_CTRL_PIN	PB1
#define PWR_CTRL_DDR	DDRB
#define PWR_CTRL_PORT	PORTB
#define TOGGLETIME		0xFFFF		/* Pre-init led toggle delay */
#define I2CDLYTIME      0xFF			/* (4)Main loop times to allow the I2C responses to finish */
#define RXDATASIZE		4			/* RX data size for WRITBUFF command */

// Global variables
byte command[16] = { 0 };           // Command received from master
byte commandLength = 0;             // Command number of bytes
word ledToggleTimer = 0;			// Pre-init led toggle timer
bool initialized = false;  			// Keeps status of initialization by master
bool exitBootloader = false;		// Flag to exit bootloader and run application
bool deleteFlash = false;			// Flag to delete flash memory and run application
//bool readyToFlash = false;			// Flag to determine if is ready to flash the memory
byte i2cDly = I2CDLYTIME;		// Delay to allow I2C execution before jumping to application
byte pageBuffer[PAGE_SIZE];		// Flash memory page buffer
word flashPageAddr = 0xFFFF;		// Flash memory page address
byte pageIX = 0;					// Flash memory page index

fptr_t RunApplication = (fptr_t)(APP_JMP_VECTOR / 2); 	// /2 for byte to word conversion // CLAWSON

// Function prototypes
void Setup(void);
void Loop(void);
//void SetCPUSpeed1MHz (void);
void SetCPUSpeed8MHz (void);
void ReceiveEvent(byte commandbytes);
void RequestEvent(void);
byte CalculateCRC(byte* block, byte blockLength);
void Reset(void);
void DisableWatchDog(void);
void DeleteFlash(void);
void __attribute__ ((noinline)) ErasePageBuffer();
void buffer_reset_vector();
void unsafe_update_page(word pageAddress);
void update_page(word pageAddress);

// Function Main
int main() {
	// Setup
	Setup();
    // Main loop
	Loop();
	// Return
	return 0;
}

// Function Setup
void Setup(void) {
	DisableWatchDog();							/* Disable watchdog to avoid continuous loop after reset */
	//SetCPUSpeed1MHz();							/* Set prescaler = 1 (System clock / 1) */
	LED_DDR |= (1 << LED_PIN);					/* Set led pin Data Direction Register for output */
	LED_PORT &= ~(1 << LED_PIN);				/* Turn led off */
	_delay_ms(250);								/* Delay to allow programming at 1 MHz after power on */
	SetCPUSpeed8MHz();							/* Set the CPU prescaler for 8 MHz */
	// Initialize I2C
	UsiTwiSlaveInit(I2C_ADDR);
	Usi_onReceiverPtr = ReceiveEvent;
	Usi_onRequestPtr = RequestEvent;
	TCCR1 = 0;	    							/* Set Timer1 off */
	cli();										/* Disable Interrupts */
}

// Function Loop
void Loop(void) {
	for (;;) {
		// Initialization check
		if (initialized == false) {
			// ============================================
			// = Blink led until is initialized by master =
			// ============================================
			if (ledToggleTimer++ >= TOGGLETIME) {
				LED_PORT ^= (1 << LED_PIN);		/* Blinks on each main loop pass at TOGGLETIME intervals */
				ledToggleTimer = 0;
			}
		}
		else {
			if (i2cDly-- <= 0) {
				// =======================================
				// = Exit bootloader and run application =
				// =======================================
				if (exitBootloader == true) {	/* Count up to i2cDly on each main loop pass to allow the I2C responses to finish */
					exitBootloader = false;
					RunApplication();
				}
				// ===========================================================================
				// = Delete application from flash memory and point reset to this bootloader =
				// ===========================================================================
				if (deleteFlash == true) {		/* Count up to i2cDly on each main loop pass to allow the I2C responses to finish */
					deleteFlash = false;
					//LED_PORT |= (1 << LED_PIN);	/* Turn on led to indicate erasing ... */
					DeleteFlash();
					RunApplication();
				}
				// ========================================================================
				// = Write received page to flash memory and prepare to receive a new one =
				// ========================================================================
				if ((pageIX == PAGE_SIZE) & (flashPageAddr != 0xFFFF)){
				//if (readyToFlash == true) {
					// FLASH ...
					//LED_PORT |= (1 << LED_PIN);	/* Turn on led to indicate writing ... */
					update_page(flashPageAddr);
					flashPageAddr = 0xFFFF;
				}
				i2cDly = I2CDLYTIME;
			}
		}
		// ==================================================
		// = I2C Interrupt Emulation ********************** =
		// = Check the USI Status Register to verify        =
		// = whether a USI start handler should be launched =
		// ==================================================
		if (USISR & (1 << USISIF)) {					
			UsiStartHandler();					/* If so, run the USI start handler ... */
			USISR |= (1 << USISIF);				/* Reset the USI start flag in USISR register to prepare for new ints */
		}
		// =====================================================
		// = I2C Interrupt Emulation ************************* =
		// = Check the USI Status Register to verify           =
		// = whether a USI counter overflow should be launched =
		// =====================================================
		if (USISR & (1 << USIOIF)) {					
			UsiOverflowHandler();				/* If so, run the USI overflow handler ... */
			USISR |= (1 << USIOIF);				/* Reset the USI overflow flag in USISR register to prepare for new ints */			
		}
	}
}

// I2C Receive Event
void ReceiveEvent(byte commandbytes) {
	// save the number of bytes sent from the master
	commandLength = commandbytes;
	// store the data from the master into the data buffer
	for (byte i = 0; i < commandLength; i++) {
		command[i] = UsiTwiReceiveByte();
	}
}

// I2C Request Event
void RequestEvent(void) {
	byte opCodeAck = ~command[0];	// Command Operation Code acknowledge => Command Bitwise "Not".
	switch (command[0]) {
		// ******************
		// * STDPB1_1 Reply *
		// ******************
		case STDPB1_1: {
			#define STDPB1_1_ACKLNG 1
			byte acknowledge[STDPB1_1_ACKLNG] = { 0 };
			acknowledge[0] = opCodeAck;
			GIMSK &= ~(1 << PCIE);						/* Disable pin change interrupt */
			TCCR1 = 0;									/* Turn off Timer1 */
			LED_DDR |= (1 << LED_PIN);					/* Set power control pin Data Direction Register for output */
			PWR_CTRL_PORT |= (1 << PWR_CTRL_PIN);		/* Turn PB1 on (Power control pin) */
			for (byte i = 0; i < STDPB1_1_ACKLNG; i++) {
				UsiTwiTransmitByte(acknowledge[i]);
			}
			break;
		}
		// ******************
		// * STDPB1_0 Reply *
		// ******************
		case STDPB1_0: {
			#define STDPB1_0_ACKLNG 1
			byte acknowledge[STDPB1_0_ACKLNG] = { 0 };
			acknowledge[0] = opCodeAck;
			GIMSK &= ~(1 << PCIE);						/* Disable pin change interrupt */
			TCCR1 = 0;									/* Turn off Timer1 */
			LED_DDR |= (1 << LED_PIN);					/* Set power control pin Data Direction Register for output */
			PWR_CTRL_PORT &= ~(1 << PWR_CTRL_PIN);		/* Turn PB1 off (Power control pin) */
			for (byte i = 0; i < STDPB1_0_ACKLNG; i++) {
				UsiTwiTransmitByte(acknowledge[i]);
			}
			break;
		}
		// ******************
		// * GETTMNLV Reply *
		// ******************
		case GETTMNLV: {
			#define GETTMNLV_ACKLNG 8
			byte acknowledge[GETTMNLV_ACKLNG] = { 0 };
			acknowledge[0] = opCodeAck;
			acknowledge[1] = 78;									// N
			acknowledge[2] = 66;									// B			
			acknowledge[3] = 84;									// T
			acknowledge[4] = TIMONEL_VER_MAJOR;						// Timonel Version Major
			acknowledge[5] = TIMONEL_VER_MINOR;						// Timonel Version Minor
			acknowledge[6] = ((BOOT_PAGE_ADDRESS & 0xFF00) >> 8);	// Timonel Base Address High Byte
			acknowledge[7] = (BOOT_PAGE_ADDRESS & 0xFF);			// Timonel Base Address Low Byte
			//acknowledge[6] = CalculateCRC(acknowledge, ackLng - 1); // Prepare CRC for Reply
			for (byte i = 0; i < GETTMNLV_ACKLNG; i++) {
				UsiTwiTransmitByte(acknowledge[i]);
			}
			break;
		}
		// ******************
		// * EXITTMNL Reply *
		// ******************
		case EXITTMNL: {
			#define EXITTMNL_ACKLNG 1
			byte acknowledge[EXITTMNL_ACKLNG] = { 0 };
			acknowledge[0] = opCodeAck;
			for (byte i = 0; i < EXITTMNL_ACKLNG; i++) {
				UsiTwiTransmitByte(acknowledge[i]);
			}
			exitBootloader = true;			
			break;
		}
		// ******************
		// * DELFLASH Reply *
		// ******************
		case DELFLASH: {
			#define DELFLASH_ACKLNG 1
			byte acknowledge[DELFLASH_ACKLNG] = { 0 };
			acknowledge[0] = opCodeAck;
			for (byte i = 0; i < DELFLASH_ACKLNG; i++) {
				UsiTwiTransmitByte(acknowledge[i]);
			}
			deleteFlash = true;
			break;
		}
		// ******************
		// * STPGADDR Reply *
		// ******************
		case STPGADDR: {
			#define STPGADDR_ACKLNG 2
			byte acknowledge[STPGADDR_ACKLNG] = { 0 };
			flashPageAddr = ((command[1] << 8) + command[2]) ;	/* Sets the flash memory page base address */
			acknowledge[0] = opCodeAck;
			acknowledge[1] = (byte)(command[1] + command[2]);	/* Returns the sum of MSB and LSB of the page address */
			for (byte i = 0; i < STPGADDR_ACKLNG; i++) {
				UsiTwiTransmitByte(acknowledge[i]);
			}
			//deleteFlash = true;
			break;
		}
		// ******************
		// * WRITBUFF Reply *
		// ******************
		case WRITBUFF: {
			#define WRITBUFF_ACKLNG 2
			byte acknowledge[WRITBUFF_ACKLNG] = { 0 };
			// At the moment, do nothing ...
			acknowledge[0] = opCodeAck;
			// Returns the sum data bytes
			for (byte i = 1; i < (RXDATASIZE + 1); i++) {
				if (pageIX < PAGE_SIZE) {
					pageBuffer[pageIX] = (command[i]);					
					pageIX++;
				}
				else {
					flashPageAddr += 64;
					pageIX = 0;
				}
				acknowledge[1] += (byte)(command[i]);
			}
			for (byte i = 0; i < WRITBUFF_ACKLNG; i++) {
				UsiTwiTransmitByte(acknowledge[i]);
			}
			if (acknowledge[1] != command[5]) {
				// ERROR
			}
			break;
		}		
		// ******************
		// * INITTINY Reply *
		// ******************
		case INITTINY: {
			#define INITTINY_ACKLNG 1
			byte acknowledge[INITTINY_ACKLNG] = { 0 };
			acknowledge[0] = opCodeAck;
			//PORTB |= (1 << LED_PIN);  // turn LED_PIN on, it will be turned off by next toggle
			PORTB &= ~(1 << PB1); // turn PB1 off (led pin)
			initialized = true;	
			for (byte i = 0; i < INITTINY_ACKLNG; i++) {
				UsiTwiTransmitByte(acknowledge[i]);
			}
			break;
		}
		default: {
			for (byte i = 0; i < commandLength; i++) {
				UsiTwiTransmitByte(UNKNOWNC);
			}
			break;
		}
	}
}

// Function CalculateCRC (CRC-8)
// byte CalculateCRC(byte* block, byte blockLength) {
//	int i;
	// byte crc = 0, data = 0;
	// for (byte i = 0; i < blockLength; i++) {
		// data = (byte)(block[i] ^ crc);	// XOR-in next input byte
		// crc = (byte)(crcTable[data]);	// Get current CRC value = remainder
	// }
	// return crc;
// }

// Function DisableWatchDog
void DisableWatchDog(void) {
    MCUSR = 0;    
    WDTCR = ((1 << WDCE) | (1 << WDE));
    WDTCR = ((1 << WDP2) | (1 << WDP1) | (1 << WDP0));
}

// Function SetCPUSpeed1MHz
// void SetCPUSpeed1MHz(void) {
	// cli();											/* Disable interrupts */
	// CLKPR = (1 << CLKPCE);							/* Mandatory for setting prescaler */
	// CLKPR = ((1 << CLKPS1) | (1 << CLKPS0)); 		/* Set prescaler 8 (System clock / 8) */
// }

// Function SetCPUSpeed8MHz
void SetCPUSpeed8MHz(void) {
	cli();											/* Disable interrupts */
	CLKPR = (1 << CLKPCE);							/* Mandatory for setting CPU prescaler */
	CLKPR = (0x00);							 		/* Set CPU prescaler 1 (System clock / 1) */
}

// Function DeleteFlash
void DeleteFlash(void) {
	ErasePageBuffer();

    // read the reset vector
    buffer_reset_vector();

    boot_spm_busy_wait();
    boot_page_erase(0); // have to erase the first page or else it will not write correctly

    unsafe_update_page(0); // restore just the initial vector

    word addr = PAGE_SIZE;
    update_page(addr);

    while (addr < BOOT_PAGE_ADDRESS) {
        boot_spm_busy_wait();
        boot_page_erase(addr);
        addr += PAGE_SIZE;
    }
}

// Function ErasePageBuffer
void __attribute__ ((noinline)) ErasePageBuffer() {
    // clear the page buffer
    for (byte i = 0; i < PAGE_SIZE; i++) {
        pageBuffer[i] = 0xFF;
    }
}

// Function buffer_reset_vector
void buffer_reset_vector() {
    // Load existing RESET vector contents into buffer.
    for(byte i = 2; i != 0; i--) {
        pageBuffer[i - 1] = pgm_read_byte(INTVECT_PAGE_ADDRESS + i - 1);
    }
}

// Function update_page
void update_page(word pageAddress) {
    // Mask out in-page address bits.
    pageAddress &= ~(PAGE_SIZE - 1);
    // Protect RESET vector if this is page 0.
    if (pageAddress == INTVECT_PAGE_ADDRESS) {
        buffer_reset_vector();
    }

    // Ignore any attempt to update boot section.
    if (pageAddress >= BOOT_PAGE_ADDRESS) {
        return;
    }

    unsafe_update_page(pageAddress);
}

// Function unsafe_update_page 
void unsafe_update_page(word pageAddress) {
    for (byte i = 0; i < PAGE_SIZE; i += 2) {
        word tempWord = ((pageBuffer[i+1] << 8) | pageBuffer[i]);
        boot_spm_busy_wait();
        boot_page_fill(pageAddress + i, tempWord); // Fill the temporary buffer with the given data
    }

    // Write page from temporary buffer to the given location in flash memory
    boot_spm_busy_wait();
    boot_page_write(pageAddress);
}

