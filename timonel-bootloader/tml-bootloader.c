/*           _                         _
 *       _  (_)                       | |
 *     _| |_ _ ____   ___  ____  _____| |
 *    (_   _) |    \ / _ \|  _ \| ___ | |
 *      | |_| | | | | |_| | | | | ____| |
 *       \__)_|_|_|_|\___/|_| |_|_____)\_)
 *
 *  Timonel - I2C Bootloader for ATtiny85 MCUs
 *  Author: Gustavo Casanova
 *  ...........................................
 *  Version: 1.1 "Sandra" / 2018-10-17
 *  gustavo.casanova@nicebots.com
 */

/* Includes */
#include <avr/boot.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include "tml-config.h"
#include "nb-usitwisl-if.h"
#include "nb-i2c-cmd.h"

/* I2C Address 08 to 35: Timonel bootloader
   I2C Address 36 to 64: Application firmware
   Each I2C node must have a unique bootloader address that corresponds
   to a defined application address, as shown in this table:
          -----------------------------------------------------------------------------------
   Boot: |08|09|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|
   Appl: |36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|
          -----------------------------------------------------------------------------------
*/
#define I2C_ADDR        8                   /* Timonel I2C address: 08 (0x08) */

/* This bootloader ... */
#define TIMONEL_VER_MJR 1                   /* Timonel version major number   */
#define TIMONEL_VER_MNR 1                   /* Timonel version major number   */

/* Configuration checks */
#if (TIMONEL_START % PAGE_SIZE != 0)
    #error "TIMONEL_START in makefile must be a multiple of chip's pagesize"
#endif

#if (PAGE_SIZE > 64)
    #error "Timonel only supports pagesizes up to 64 bytes"
#endif

#if (!(AUTO_TPL_CALC) && !(CMD_STPGADDR))
    #error "If the AUTO_TPL_CALC option is disabled, then CMD_STPGADDR must be enabled in tml-config.h!"
#endif
                                
#ifndef F_CPU
    #define F_CPU 8000000UL                 /* Default CPU speed */
#endif

#if (RXDATASIZE > 8)
    #pragma GCC warning "Do not set transmission data size too high to avoid hurting I2C reliability!"
#endif

#if ((CYCLESTOEXIT > 0) && (CYCLESTOEXIT < 20))
    #pragma GCC warning "Do not set CYCLESTOEXIT too low, it could make difficult for I2C master to initialize on time!"
#endif

// Type definitions
typedef uint8_t byte;
typedef uint16_t word;
typedef void (* const fptr_t)(void);

// Global variables
byte command[(RXDATASIZE * 2) + 2] = { 0 }; /* Command received from I2C master */
byte statusRegister = 0;                    /* Bit: 8,7,6,5: Not used; 4: exit; 3: delete flash; 2, 1: initialized */
word flashPageAddr = 0x0000;                /* Flash memory page address */
byte pageIX = 0;                            /* Flash memory page index */
#if AUTO_TPL_CALC
byte appResetLSB = 0xFF;                    /* Application first byte  */
byte appResetMSB = 0xFF;                    /* Application second byte*/
#endif /* AUTO_TPL_CALC */

// Jump to trampoline
static const fptr_t RunApplication = (const fptr_t)((TIMONEL_START - 2) / 2);
#if !(USE_WDT_RESET)
// Restart this bootloader
static const fptr_t RestartTimonel = (const fptr_t)(TIMONEL_START / 2);
#endif /* !USE_WDT_RESET */

// Prototypes
void ReceiveEvent(byte commandBytes);
void RequestEvent(void);

// Function Main
int main() {
    /*  ___________________
       |                   | 
       |    Setup Block    |
       |___________________|
    */
    MCUSR = 0;                              /* Disable watchdog */
    WDTCR = ((1 << WDCE) | (1 << WDE));
    WDTCR = ((1 << WDP2) | (1 << WDP1) | (1 << WDP0));
    cli();                                  /* Disable Interrupts */
#if ENABLE_LED_UI
    LED_UI_DDR |= (1 << LED_UI_PIN);        /* Set led pin data direction register for output */
#endif /* ENABLE_LED_UI */
#if SET_PRESCALER
    CLKPR = (1 << CLKPCE);                  /* Set the CPU prescaler division factor = 1 */
    CLKPR = (0x00);
#endif /* SET_PRESCALER */
    UsiTwiSlaveInit(I2C_ADDR);              /* Initialize I2C */
    Usi_onReceiverPtr = ReceiveEvent;       /* I2C Receive Event */
    Usi_onRequestPtr = RequestEvent;        /* I2C Request Event */
    __SPM_REG = (_BV(CTPB) | \
    _BV(__SPM_ENABLE));                     /* Clear temporary page buffer */
    asm volatile("spm");
    word dlyCounter = CYCLESTOWAIT;
    byte exitDly = CYCLESTOEXIT;            /* Delay to exit bootloader and run the application if not initialized */
    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */
    for (;;) {
        if (dlyCounter-- <= 0) {
            dlyCounter = CYCLESTOWAIT;
            // Initialization check
#if !(TWO_STEP_INIT)
            if ((statusRegister & (1 << ST_INIT_1)) != (1 << ST_INIT_1)) {
#endif /* !TWO_STEP_INIT */
#if TWO_STEP_INIT
            if ((statusRegister & ((1 << ST_INIT_1) + (1 << ST_INIT_2))) != \
            ((1 << ST_INIT_1) + (1 << ST_INIT_2))) {
#endif /* TWO_STEP_INIT */
                // ===========================================
                // = Run this until is initialized by master =
                // ===========================================
#if ENABLE_LED_UI               
                LED_UI_PORT ^= (1 << LED_UI_PIN);   /* Blinks on each main loop pass at CYCLESTOWAIT intervals */
#endif /* ENABLE_LED_UI */
                if (exitDly-- == 0) {
                    RunApplication();               /* Count from CYCLESTOEXIT to 0, then exit to the application */
                }
            }
            else {
                // =======================================
                // = Exit bootloader and run application =
                // =======================================
                if ((statusRegister & (1 << ST_EXIT_TML)) == (1 << ST_EXIT_TML) ) {
                    asm volatile("cbr r31, 0x80");          /* Clear bit 7 of r31 */
                    RunApplication();                       /* Exit to the application */
                }
                // ========================================
                // = Delete application from flash memory =
                // ========================================
                if ((statusRegister & (1 << ST_DEL_FLASH)) == (1 << ST_DEL_FLASH)) {
#if ENABLE_LED_UI                   
                    LED_UI_PORT |= (1 << LED_UI_PIN);       /* Turn led on to indicate erasing ... */
#endif /* ENABLE_LED_UI */
                    word pageAddress = TIMONEL_START;       /* Erase flash ... */
                    while (pageAddress != RESET_PAGE) {
                        pageAddress -= PAGE_SIZE;
                        boot_page_erase(pageAddress);
                    }
#if !(USE_WDT_RESET)
                    RestartTimonel();                       /* Exit to the application, in this case restarts the bootloader */
#else
                    wdt_enable(WDTO_15MS);                  /* RESETTING ... WARNING!!! */
                    for (;;) {};
#endif /* !USE_WDT_RESET */                    
                }
                // ========================================================================
                // = Write received page to flash memory and prepare to receive a new one =
                // ========================================================================
#if APP_USE_TPL_PG
                if ((pageIX == PAGE_SIZE) & (flashPageAddr < TIMONEL_START)) {
#else
                if ((pageIX == PAGE_SIZE) & (flashPageAddr < TIMONEL_START - PAGE_SIZE)) {
#endif /* APP_USE_TPL_PG */
#if ENABLE_LED_UI
                    LED_UI_PORT ^= (1 << LED_UI_PIN);   /* Turn led on and off to indicate writing ... */
#endif /* ENABLE_LED_UI */
#if FORCE_ERASE_PG
                    boot_page_erase(flashPageAddr);
#endif /* FORCE_ERASE_PG */                    
                    boot_page_write(flashPageAddr);
#if AUTO_TPL_CALC
                    word tpl = (((~((TIMONEL_START >> 1) - ((((appResetMSB << 8) | appResetLSB) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
                    if (flashPageAddr == RESET_PAGE) {    /* Calculate and write trampoline */
                        for (int i = 0; i < PAGE_SIZE - 2; i += 2) {
                            boot_page_fill((TIMONEL_START - PAGE_SIZE) + i, 0xFFFF);
                        }
                        boot_page_fill((TIMONEL_START - 2), tpl);
                        boot_page_write(TIMONEL_START - PAGE_SIZE);                        
                    }
#if APP_USE_TPL_PG
                    if ((flashPageAddr) == (TIMONEL_START - PAGE_SIZE)) {
                        // - Read the previous page to the bootloader start, write it to the temporary buffer.
                        const __flash unsigned char * flashAddr;
                        for (byte i = 0; i < PAGE_SIZE - 2; i += 2) {
                            flashAddr = (void *)((TIMONEL_START - PAGE_SIZE) + i);
                            word pgData = (*flashAddr & 0xFF);
                            pgData += ((*(++flashAddr) & 0xFF) << 8); 
                            boot_page_fill((TIMONEL_START - PAGE_SIZE) + i, pgData);
                        }
                        // - Check if the last two bytes of the trampoline page are 0xFF.
                        flashAddr = (void *)(TIMONEL_START - 2);
                        word pgData = (*flashAddr & 0xFF);
                        pgData += ((*(++flashAddr) & 0xFF) << 8);
                        if (pgData == 0xFFFF) {
                            // -- If yes, then the application fits in memory, flash the trampoline bytes again.                            
                            boot_page_fill(TIMONEL_START - 2, tpl);
                            boot_page_erase(TIMONEL_START - PAGE_SIZE);
                            boot_page_write(TIMONEL_START - PAGE_SIZE);
                        }
                        else {
                            // -- If no, it means that the application is too big for this setup, erase it! 
                            statusRegister |= (1 << ST_DEL_FLASH);
                        }
                    }
#endif /* APP_USE_TPL_PG */
#endif /* AUTO_TPL_CALC */
#if !(CMD_STPGADDR)
                    flashPageAddr += PAGE_SIZE;
#endif /* !CMD_STPGADDR */
                    pageIX = 0;
                }
            }
        }
        /* ..................................................
           . I2C Interrupt Emulation ...................... .
           . Check the USI Status Register to verify        .
           . whether a USI start handler should be launched .
           ..................................................
        */
        if (USISR & (1 << USISIF)) {
            UsiStartHandler();      /* If so, run the USI start handler ... */
            USISR |= (1 << USISIF); /* Reset the USI start flag in USISR register to prepare for new ints */
        }
        /* .....................................................
           . I2C Interrupt Emulation ......................... .
           . Check the USI Status Register to verify           .
           . whether a USI counter overflow should be launched .
           .....................................................
        */
        if (USISR & (1 << USIOIF)) {
            UsiOverflowHandler();   /* If so, run the USI overflow handler ... */
            USISR |= (1 << USIOIF); /* Reset the USI overflow flag in USISR register to prepare for new ints */
        }
    }
    return 0;
}

// I2C Receive Event
void ReceiveEvent(byte commandBytes) {
    for (byte i = 0; i < commandBytes; i++) {
        command[i] = UsiTwiReceiveByte();                           /* Store the data sent by the I2C master in the data buffer */
    }
}

// I2C Request Event
void RequestEvent(void) {
    byte opCodeAck = ~command[0];                                   /* Command code reply => Command Bitwise "Not" */
    switch (command[0]) {
        // ******************
        // * GETTMNLV Reply *
        // ******************
        case GETTMNLV: {
#if CHECK_EMPTY_FL
            #define GETTMNLV_RPLYLN 10
#else
            #define GETTMNLV_RPLYLN 9
#endif /* CHECK_EMPTY_FL */
            const __flash unsigned char * flashAddr;
            flashAddr = (void *)(TIMONEL_START - 1); 
            byte reply[GETTMNLV_RPLYLN] = { 0 };
            reply[0] = opCodeAck;
            reply[1] = ID_CHAR_3;                                   /* T */            
            reply[2] = TIMONEL_VER_MJR;                             /* Major version number */
            reply[3] = TIMONEL_VER_MNR;                             /* Minor version number */
            reply[4] = TML_FEATURES;                                /* Optional features */
            reply[5] = ((TIMONEL_START & 0xFF00) >> 8);             /* Start address MSB */
            reply[6] = (TIMONEL_START & 0xFF);                      /* Start address LSB */
            reply[7] = (*flashAddr & 0xFF);                         /* Trampoline second byte (MSB) */
            reply[8] = (*(--flashAddr) & 0xFF);                     /* Trampoline first byte (LSB) */
#if CHECK_EMPTY_FL
            for (word mPos = 0; mPos < 100; mPos++) {               /* Check the first 100 memory positions to determine if  */
                flashAddr = (void *)(mPos);                         /* there is an application (or some other data) loaded.  */
                reply[9] += (byte)~(*flashAddr);                    
            }
#endif /* CHECK_EMPTY_FL */
#if !(TWO_STEP_INIT)
            //statusRegister |= (1 << (ST_INIT_1)) | (1 << (ST_INIT_2));  /* Single-step init */
            statusRegister |= (1 << (ST_INIT_1));                   /* Single-step init */
#endif /* !TWO_STEP_INIT */
#if TWO_STEP_INIT
            statusRegister |= (1 << ST_INIT_2);                     /* Two-step init step 2: receive GETTMNLV command */
#endif /* TWO_STEP_INIT */
#if ENABLE_LED_UI
            LED_UI_PORT &= ~(1 << LED_UI_PIN);                      /* Turn led off to indicate initialization */
#endif /* ENABLE_LED_UI */
            for (byte i = 0; i < GETTMNLV_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            break;
        }
        // ******************
        // * EXITTMNL Reply *
        // ******************
        case EXITTMNL: {
            UsiTwiTransmitByte(opCodeAck);
            statusRegister |= (1 << ST_EXIT_TML);
            break;
        }
        // ******************
        // * DELFLASH Reply *
        // ******************
        case DELFLASH: {
            UsiTwiTransmitByte(opCodeAck);
            statusRegister |= (1 << ST_DEL_FLASH);
            break;
        }
#if (CMD_STPGADDR || !(AUTO_TPL_CALC))
        // ******************
        // * STPGADDR Reply *
        // ******************
        case STPGADDR: {
            #define STPGADDR_RPLYLN 2
            byte reply[STPGADDR_RPLYLN] = { 0 };
            flashPageAddr = ((command[1] << 8) + command[2]);       /* Sets the flash memory page base address */
            flashPageAddr &= ~(PAGE_SIZE - 1);                      /* Keep only pages' base addresses */
            reply[0] = opCodeAck;
            reply[1] = (byte)(command[1] + command[2]);             /* Returns the sum of MSB and LSB of the page address */
            for (byte i = 0; i < STPGADDR_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            break;
        }
#endif /* CMD_STPGADDR || !AUTO_TPL_CALC */
        // ******************
        // * WRITPAGE Reply *
        // ******************
        case WRITPAGE: {
            #define WRITPAGE_RPLYLN 2
            byte reply[WRITPAGE_RPLYLN] = { 0 };
            reply[0] = opCodeAck;
            if ((flashPageAddr + pageIX) == RESET_PAGE) {
#if AUTO_TPL_CALC
                appResetLSB = command[1];
                appResetMSB = command[2];
#endif /* AUTO_TPL_CALC */
                // Modify the reset vector to point to this bootloader
                boot_page_fill((RESET_PAGE), (0xC000 + ((TIMONEL_START / 2) - 1)));
                reply[1] += (byte)((command[2]) + command[1]);      /* Reply checksum accumulator */
                pageIX += 2;
                for (byte i = 3; i < (RXDATASIZE + 1); i += 2) {
                    boot_page_fill((flashPageAddr + pageIX), ((command[i + 1] << 8) | command[i]));
                    reply[1] += (byte)((command[i + 1]) + command[i]);
                    pageIX += 2;
                }                
            }
            else {
                for (byte i = 1; i < (RXDATASIZE + 1); i += 2) {
                    boot_page_fill((flashPageAddr + pageIX), ((command[i + 1] << 8) | command[i]));
                    reply[1] += (byte)((command[i + 1]) + command[i]);
                    pageIX += 2;
                }
            }
            if ((reply[1] != command[RXDATASIZE + 1]) || (pageIX > PAGE_SIZE)) {
                statusRegister |= (1 << ST_DEL_FLASH);              /* Safety payload deletion ... */
                reply[1] = 0;
            }
            for (byte i = 0; i < WRITPAGE_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            break;
        }
        
#if CMD_READFLASH
        // ******************
        // * READFLSH Reply *
        // ******************
        case READFLSH: {
			// command[0] : OpCode
			// command[1] : Flash Position MSB
            // command[2] : Flash Position LSB
			// command[3] : Requested Bytes
            // command[4] : Checksum
            byte reply[STPGADDR_RPLYLN] = { 0 };
            flashPageAddr = ((command[1] << 8) + command[2]);       /* Sets the flash memory page base address */

            reply[0] = opCodeAck;
            reply[1] = (byte)(command[1] + command[2]) + command[3]; /* Returns the sum of MSB, LSB and requested bytes */
            for (byte i = 0; i < READFLSH_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            break;
            
		// case READPAGE: {
			// // command[0] : OpCode
			// // command[1] : Start Position
			// // command[2] : Requested Bytes
			// uint8_t ix = command[1];			/* Second byte received determines start of reply data */
			// const uint8_t ackLng = (command[2] + 2);	/* Third byte received determines the size of reply data */
			// uint8_t reply[ackLng];
			// reply[ackLng - 1] = 0;				/* Checksum initialization */
			// reply[0] = opCodeAck;
			// if ((ix > 0) & (ix <= PAGE_SIZE) & (command[2] >= 1) & (command[2] <= MAXBUFFERTXLN * 2)) {
				// uint8_t j = 1;
				// reply[ackLng - 1] = 0;
				// for (uint8_t i = 1; i < command[2] + 1; i++) {
					// reply[j] = pageBuffer[ix + i - 2];	/* Data bytes in reply */
					// reply[ackLng - 1] += reply[j];		/* Checksum accumulator to be sent in the last byte of the reply */
					// j++;
				// }
				// //reply[ackLng - 1] = CalculateCRC(reply, ackLng - 1);	/* Prepare CRC for Reply */
				// for (uint8_t i = 0; i < ackLng; i++) {
					// UsiTwiTransmitByte(reply[i]);
				// }
			// }
			// else {
				// UsiTwiTransmitByte(UNKNOWNC);		/* Incorrect operand value received */
			// }
			// break;
		// }            
            
        }
#endif /* CMD_READFLASH */
        
#if TWO_STEP_INIT
        // ******************
        // * INITTINY Reply *
        // ******************
        case INITTINY: {
            statusRegister |= (1 << ST_INIT_1);                     /* Two-step init step 1: receive INITTINY command */
            UsiTwiTransmitByte(opCodeAck);
            break;
        }
#endif /* TWO_STEP_INIT */
        default: {
            UsiTwiTransmitByte(UNKNOWNC);
            break;
        }
    }
}
