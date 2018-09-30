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
 *  Version: 0.9 / 2018-09-29
 *  gustavo.casanova@nicebots.com
 */

/* Includes */
//#include <avr/io.h>
#include <avr/boot.h>
#include <avr/wdt.h>
#include "tml-config.h"
#include "nb-usitwisl-if.h"
#include "nb-i2c-cmd.h"

/* This bootloader ... */
#define I2C_ADDR        0x15                /* Timonel I2C Address: 0x15 = 21 */
#define TIMONEL_VER_MJR 0                   /* Timonel version major number */
#define TIMONEL_VER_MNR 9                   /* Timonel version major number */

#if (TIMONEL_START % PAGE_SIZE != 0)
    #error "TIMONEL_START in makefile must be a multiple of chip's pagesize"
#endif

#if (PAGE_SIZE > 64)
    #error "Timonel only supports pagesizes up to 64 bytes"
#endif
                                
#ifndef F_CPU
    #define F_CPU 8000000UL                 /* Default CPU speed for delay.h */
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
typedef void (*fptr_t)(void);

// Global variables
byte command[(RXDATASIZE * 2) + 2] = { 0 }; /* Command received from I2C master */
byte statusRegister = 0;                    /* Bit: 8,7,6,5: Not used, 4: exit, 3: delete flash, 2, 1: initialized */
word flashPageAddr = 0x0000;                /* Flash memory page address */
byte pageIX = 0;                            /* Flash memory page index */
byte appResetLSB = 0xFF;
byte appResetMSB = 0xFF;

// Jump to trampoline
fptr_t RunApplication = (fptr_t)((TIMONEL_START - 2) / 2);

// Function prototypes
void ReceiveEvent(byte commandBytes);
void RequestEvent(void);
void Reset(void);

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
    LED_UI_DDR |= (1 << LED_UI_PIN);        /* Set led pin Data Direction Register for output */
#endif
    CLKPR = (1 << CLKPCE);                  /* Set the CPU prescaler for 8 MHz */
    CLKPR = (0x00);    
    UsiTwiSlaveInit(I2C_ADDR);              /* Initialize I2C */
    Usi_onReceiverPtr = ReceiveEvent;       /* I2C Receive Event declaration */
    Usi_onRequestPtr = RequestEvent;        /* I2C Request Event declaration */
    statusRegister = (1 << ST_APP_READY);   /* In principle, assume that there is a valid app in memory */
    word dlyCounter = TOGGLETIME;
    byte exitDly = CYCLESTOEXIT;            /* Delay to exit Timonel and run the application if not initialized */
    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */
    
    for (;;) {
        // Initialization check
        if ((statusRegister & ((1 << ST_INIT_1) + (1 << ST_INIT_2))) != \
        ((1 << ST_INIT_1) + (1 << ST_INIT_2))) {
            // ============================================
            // = Blink led until is initialized by master =
            // ============================================
            if (dlyCounter-- <= 0) {
#if ENABLE_LED_UI               
                LED_UI_PORT ^= (1 << LED_UI_PIN);   /* Blinks on each main loop pass at TOGGLETIME intervals */
#endif
                dlyCounter = TOGGLETIME;
                if (exitDly-- == 0) {
                    RunApplication();
                }
            }
        }
        else {
            if (dlyCounter-- <= 0) {                /* Decrease dlyCounter on each main loop pass until it */
                //                                  /* reaches 0 before running code to allow I2C replies  */
                // =======================================
                // = Exit bootloader and run application =
                // =======================================
                if ((statusRegister & ((1 << ST_EXIT_TML) + (1 << ST_APP_READY))) == \
                ((1 << ST_EXIT_TML) + (1 << ST_APP_READY))) {
                    RunApplication();                       /* Launch application */
                }
                if ((statusRegister & ((1 << ST_EXIT_TML) + (1 << ST_APP_READY))) == \
                (1 << ST_EXIT_TML)) {               
                    statusRegister |= (1 << ST_DEL_FLASH);  /* Set Erase flash */
                }
                // ========================================
                // = Delete application from flash memory =
                // ========================================
                if ((statusRegister & (1 << ST_DEL_FLASH)) == (1 << ST_DEL_FLASH)) {
#if ENABLE_LED_UI                   
                    LED_UI_PORT |= (1 << LED_UI_PIN);       /* Turn led on to indicate erasing ... */
#endif                  
                    word pageAddress = TIMONEL_START;       /* Erase Flash ... */
                    while (pageAddress != RESET_PAGE) {
                        pageAddress -= PAGE_SIZE;
                        boot_page_erase(pageAddress);
                    }
                    wdt_enable(WDTO_15MS);                  /* RESETTING ... WARNING!!! */
                    for (;;) {};
                }
                // ========================================================================
                // = Write received page to flash memory and prepare to receive a new one =
                // ========================================================================
                if ((pageIX == PAGE_SIZE) & (flashPageAddr < TIMONEL_START)) {
#if ENABLE_LED_UI                   
                    LED_UI_PORT ^= (1 << LED_UI_PIN);   /* Turn led on and off to indicate writing ... */
#endif

                    boot_page_erase(flashPageAddr);
                    boot_page_write(flashPageAddr);

                    word tpl = (((~((TIMONEL_START >> 1) - ((((appResetMSB << 8) | appResetLSB) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
                    
                    if (flashPageAddr == RESET_PAGE) {    /* Calculate and Write Trampoline */
                        for (int i = 0; i < PAGE_SIZE - 2; i += 2) {
                            boot_page_fill((TIMONEL_START - PAGE_SIZE) + i, 0xffff);
                        }
                        //boot_page_fill((TIMONEL_START - 2), (((~((TIMONEL_START >> 1) - ((((appResetMSB << 8) | appResetLSB) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000));

                        boot_page_fill((TIMONEL_START - 2), tpl);
                        boot_page_write(TIMONEL_START - PAGE_SIZE);                        
                    }

                    if ((flashPageAddr) == (TIMONEL_START - PAGE_SIZE)) {
                        /*
                        Read the last page before Timonel start
                        Write it to the temporary buffer
                        Check if the last two bytes are 0xFF
                        If yes, then the the application fits in memory, flash the trampoline again
                        If no, it means that the application is too big, erase the application
                        TODO: Implement ALLOW_USE_TPL_PG (allow use trampoline page)
                        TODO: Implement AUTO_TPL_CALC (auto-trampoline calculation & flash)
                        TODO: Implement a GETTMNLV reply code that indicates the commands available
                        */
                        
                        const __flash unsigned char * flashAddr;

                        for (byte i = 0; i < PAGE_SIZE - 2; i += 2) {
                            flashAddr = (void *)((TIMONEL_START - PAGE_SIZE) + i);
                            word pgData = (*flashAddr & 0xFF);
                            pgData += ((*(++flashAddr) & 0xFF) << 8); 
                            boot_page_fill((TIMONEL_START - PAGE_SIZE) + i, pgData);
                        }
                        boot_page_fill(TIMONEL_START - 2, tpl);
                        boot_page_erase(TIMONEL_START - PAGE_SIZE);
                        boot_page_write(TIMONEL_START - PAGE_SIZE);
                    }   
                    
                    flashPageAddr += PAGE_SIZE;
                    pageIX = 0;
                }
                //dlyCounter = I2CDLYTIME;
            }
        }
        // ==================================================
        // = I2C Interrupt Emulation ********************** =
        // = Check the USI Status Register to verify        =
        // = whether a USI start handler should be launched =
        // ==================================================
        if (USISR & (1 << USISIF)) {
            UsiStartHandler();      /* If so, run the USI start handler ... */
            USISR |= (1 << USISIF); /* Reset the USI start flag in USISR register to prepare for new ints */
        }
        // =====================================================
        // = I2C Interrupt Emulation ************************* =
        // = Check the USI Status Register to verify           =
        // = whether a USI counter overflow should be launched =
        // =====================================================
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
    byte opCodeAck = ~command[0];                                   /* Command Operation Code reply => Command Bitwise "Not" */
    switch (command[0]) {
        // ******************
        // * GETTMNLV Reply *
        // ******************
        case GETTMNLV: {
            #define GETTMNLV_RPLYLN 9
            //const __flash unsigned char * flashAddr;
            //flashAddr = (void *)(TIMONEL_START - 1);
            byte reply[GETTMNLV_RPLYLN] = { 0 };
            reply[0] = opCodeAck;
            reply[1] = ID_CHAR_3;                                   /* T */            
            reply[2] = TIMONEL_VER_MJR;                             /* Timonel Major version number */
            reply[3] = TIMONEL_VER_MNR;                             /* Timonel Minor version number */
            reply[4] = ((TIMONEL_START & 0xFF00) >> 8);             /* Timonel Base Address MSB */
            reply[5] = (TIMONEL_START & 0xFF);                      /* Timonel Base Address LSB */
            //reply[6] = (*flashAddr & 0xFF);                       /* Trampoline Second Byte MSB */
            //reply[7] = (*(--flashAddr) & 0xFF);                   /* Trampoline First Byte LSB */
            // for (uint16_t i = 0; i < 100; i++) {
                // flashAddr = (void *)(i);
                // reply[8] += (byte)~(*flashAddr);                 /* Check the first 100 bytes to determine if there is an app flashed */
            // }
            reply[6] = 0;
            reply[7] = 0;
            reply[8] = 0;
#if !(TWO_STEP_INIT)
            statusRegister |= (1 << (ST_INIT_1)) | (1 << (ST_INIT_2));  /* Single-step init */
#endif
#if TWO_STEP_INIT
            statusRegister |= (1 << ST_INIT_2);                     /* Two-step init step 2: receive GETTMNLV command */
#endif
#if ENABLE_LED_UI
            LED_UI_PORT &= ~(1 << LED_UI_PIN);                      /* Turn led off to indicate initialization */
#endif 
            for (byte i = 0; i < GETTMNLV_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            break;
        }
        // ******************
        // * EXITTMNL Reply *
        // ******************
        case EXITTMNL: {
            #define EXITTMNL_RPLYLN 1
            UsiTwiTransmitByte(opCodeAck);
            statusRegister |= (1 << ST_EXIT_TML);
            break;
        }
        // ******************
        // * DELFLASH Reply *
        // ******************
        case DELFLASH: {
            #define DELFLASH_RPLYLN 1
            UsiTwiTransmitByte(opCodeAck);
            statusRegister |= (1 << ST_DEL_FLASH);
            break;
        }
#if CMD_STPGADDR        
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
#endif      
        // ******************
        // * WRITPAGE Reply *
        // ******************
        case WRITPAGE: {
            #define WRITPAGE_RPLYLN 2
            uint8_t reply[WRITPAGE_RPLYLN] = { 0 };
            reply[0] = opCodeAck;
            if ((flashPageAddr + pageIX) == RESET_PAGE) {
                //SPMCSR |= (1 << CTPB);                            /* Clear temporary page buffer */
                appResetLSB = command[1];
                appResetMSB = command[2];
                boot_page_fill((RESET_PAGE), (0xC000 + ((TIMONEL_START / 2) - 1)));
                reply[1] += (uint8_t)((command[2]) + command[1]);   /* Reply checksum accumulator */
                pageIX += 2;
                for (uint8_t i = 3; i < (RXDATASIZE + 1); i += 2) {
                    boot_page_fill((flashPageAddr + pageIX), ((command[i + 1] << 8) | command[i]));
                    reply[1] += (uint8_t)((command[i + 1]) + command[i]);
                    pageIX += 2;
                }                
            }
            else {
                for (uint8_t i = 1; i < (RXDATASIZE + 1); i += 2) {
                    boot_page_fill((flashPageAddr + pageIX), ((command[i + 1] << 8) | command[i]));
                    reply[1] += (uint8_t)((command[i + 1]) + command[i]);
                    pageIX += 2;
                }
            }
            if ((reply[1] != command[RXDATASIZE + 1]) || (pageIX > PAGE_SIZE)) {
                //statusRegister &= ~(1 << ST_APP_READY);           /* Payload received with errors, don't run it !!! */
                statusRegister |= (1 << ST_DEL_FLASH);              /* Safety payload deletion ... */
                reply[1] = 0;
            }
            for (uint8_t i = 0; i < WRITPAGE_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            break;
        }
#if TWO_STEP_INIT
        // ******************
        // * INITTINY Reply *
        // ******************
        case INITTINY: {
            #define INITTINY_RPLYLN 1
            statusRegister |= (1 << ST_INIT_1);                     /* Two-step init step 1: receive INITTINY command */
            UsiTwiTransmitByte(opCodeAck);
            break;
        }
#endif
        default: {
            UsiTwiTransmitByte(UNKNOWNC);
            break;
        }
    }
}
