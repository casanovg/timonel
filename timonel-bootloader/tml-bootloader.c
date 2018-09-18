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
 *  Version: 0.83 / 2018-09-17
 *  gustavo.casanova@nicebots.com
 */

/* Includes */
//#include <avr/io.h>
#include <avr/boot.h>
#include "tml-config.h"
#include "nb-usitwisl-if.h"
#include "nb-i2c-cmd.h"

/* This bootloader ... */
#define I2C_ADDR        0x15                /* Timonel I2C Address: 0x15 = 21 */
#define TIMONEL_VER_MJR 0                   /* Timonel version major number */
#define TIMONEL_VER_MNR 85                  /* Timonel version major number */

#if TIMONEL_START % PAGE_SIZE != 0
    #error "TIMONEL_START in makefile must be a multiple of chip's pagesize"
#endif

#if (PAGE_SIZE > 64)
    #error "Timonel only supports pagesizes up to 64 bytes"
#endif
                                
#ifndef F_CPU
    #define F_CPU 8000000UL                 /* Default CPU speed for delay.h */
#endif

#if (RXDATASIZE > 8)
    #pragma GCC warning "Do not set transmission data too high to avoid hurting I2C reliability!"
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
byte commandLength = 0;                     /* Command number of bytes */
word ledToggleTimer = 0;                    /* Pre-init led toggle timer */
byte statusRegister = 0;                    /* Bit: 8,7,6,5: Not used, 4: exit, 3: delete flash, 2, 1: initialized */
word i2cDly = I2CDLYTIME;                   /* Delay to allow I2C execution before jumping to application */
byte exitDly = CYCLESTOEXIT;                /* Delay to exit Timonel and run the application if not initialized */
//byte pageBuffer[PAGE_SIZE];               /* Flash memory page buffer */
word flashPageAddr = 0x0000;                /* Flash memory page address */
byte pageIX = 0;                            /* Flash memory page index */
byte tplJumpLowByte = 0;                    /* Trampoline jump address LSB */
byte tplJumpHighByte = 0;                   /* Trampoline jump address MSB */

byte appResetLSB = 0;
byte appResetMSB = 0;

word jumpOffset = 0;

// Jump to trampoline
fptr_t RunApplication = (fptr_t)((TIMONEL_START - 2) / 2);

// Function prototypes
void SetCPUSpeed8MHz(void);
void ReceiveEvent(byte commandBytes);
void RequestEvent(void);
void Reset(void);
void DisableWatchDog(void);
void DeleteFlash(void);
void ClearPageBuffer();
void FixResetVector();
void FlashRaw(word pageAddress);
void FlashPage(word pageAddress);
void CreateTrampoline(void);
void CalculateTrampoline(byte applJumpLowByte, byte applJumpHighByte);

// Function Main
int main() {
    /*  ___________________
       |                   | 
       |    Setup Block    |
       |___________________|
    */
    DisableWatchDog();                      /* Disable watchdog to avoid continuous loop after reset */
#if ENABLE_LED_UI
    LED_UI_DDR |= (1 << LED_UI_PIN);        /* Set led pin Data Direction Register for output */
#endif
    SetCPUSpeed8MHz();                      /* Set the CPU prescaler for 8 MHz */
    UsiTwiSlaveInit(I2C_ADDR);              /* Initialize I2C */
    Usi_onReceiverPtr = ReceiveEvent;       /* I2C Receive Event declaration */
    Usi_onRequestPtr = RequestEvent;        /* I2C Request Event declaration */
    statusRegister = (1 << ST_APP_READY);   /* In principle, we assume that there is a valid app in memory */
    cli();                                  /* Disable Interrupts */
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
            if (ledToggleTimer++ >= TOGGLETIME) {
#if ENABLE_LED_UI               
                LED_UI_PORT ^= (1 << LED_UI_PIN);   /* Blinks on each main loop pass at TOGGLETIME intervals */
#endif
                ledToggleTimer = 0;
                if (exitDly-- == 0) {
                    RunApplication();
                }
            }
        }
        else {
            if (i2cDly-- <= 0) {                    /* Decrease i2cDly on each main loop pass until it    */
                //                                  /* reaches 0 before running code to allow I2C replies */
                // =======================================
                // = Exit bootloader and run application =
                // =======================================
                if ((statusRegister & ((1 << ST_EXIT_TML) + (1 << ST_APP_READY))) == \
                ((1 << ST_EXIT_TML) + (1 << ST_APP_READY))) {
                    RunApplication();
                }
                if ((statusRegister & ((1 << ST_EXIT_TML) + (1 << ST_APP_READY))) == \
                (1 << ST_EXIT_TML)) {
                    DeleteFlash();
                }
                // ===========================================================================
                // = Delete application from flash memory and point reset to this bootloader =
                // ===========================================================================
                if ((statusRegister & (1 << ST_DEL_FLASH)) == (1 << ST_DEL_FLASH)) {
#if ENABLE_LED_UI                   
                    LED_UI_PORT |= (1 << LED_UI_PIN);   /* Turn led on to indicate erasing ... */
#endif                  
                    DeleteFlash();
                    RunApplication();                   /* Since there is no app anymore, this resets to the bootloader */
                }
                // ========================================================================
                // = Write received page to flash memory and prepare to receive a new one =
                // ========================================================================
                if ((pageIX == PAGE_SIZE) /* & (flashPageAddr != 0xFFFF)*/) {
#if ENABLE_LED_UI                   
                    LED_UI_PORT ^= (1 << LED_UI_PIN);   /* Turn led on and off to indicate writing ... */
#endif
                    if ((flashPageAddr) == RESET_PAGE) {
                        //CalculateTrampoline(appResetLSB, appResetMSB);
                        //jumpOffset = (~(TIMONEL_START - (((((appResetLSB << 8) + appResetMSB) + 1) & 0x0FFF) << 1)) >> 1);
                        //jumpOffset = TIMONEL_START - ((word)(((appResetMSB << 8) + appResetLSB) & 0x0FFF) >>)
                        jumpOffset++;
                        
                        //boot_page_fill(RESET_PAGE, ((0xC000 + (TIMONEL_START / 2) - 1) >> 8) + ((0xC000 + (TIMONEL_START / 2) - 1) & 0xff));
                        boot_spm_busy_wait();
                        boot_page_fill(RESET_PAGE, (word)(0xC000 + ((TIMONEL_START / 2) - 1)));
                        //---boot_spm_busy_wait();
                        //---boot_page_fill(0, 0xAABB);
                    }                    
                    
                    boot_spm_busy_wait();
                    boot_page_write(flashPageAddr);
                    
                    if ((flashPageAddr) == RESET_PAGE) {
                        //CreateTrampoline();
                        word address = (TIMONEL_START - PAGE_SIZE);
                        for (int i = 0; i < PAGE_SIZE - 2; i += 2) {
                            boot_spm_busy_wait();
                            boot_page_fill(address + i, 0xffff);
                        }
                        //boot_page_fill((TIMONEL_START - 2), (word)((word)(tplJumpHighByte << 8) + tplJumpLowByte));
                        //boot_page_fill((TIMONEL_START - 2), 0xc2ef);
                        boot_page_fill((TIMONEL_START - 2), (word)(0xC000 + jumpOffset));
                        boot_spm_busy_wait();
                        boot_page_write(address);                        
                    }
                    
                    flashPageAddr += PAGE_SIZE;
                    pageIX = 0;
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
    // Return
    return 0;
}

// I2C Receive Event
void ReceiveEvent(byte commandBytes) {
    commandLength = commandBytes;                       /* Save the number of bytes sent by the I2C master */
    for (byte i = 0; i < commandLength; i++) {
        command[i] = UsiTwiReceiveByte();               /* Store the data sent by the I2C master in the data buffer */
    }
}

// I2C Request Event
void RequestEvent(void) {
    byte opCodeAck = ~command[0];                       /* Command Operation Code reply => Command Bitwise "Not" */
    switch (command[0]) {
        // ******************
        // * GETTMNLV Reply *
        // ******************
        case GETTMNLV: {
            #define GETTMNLV_RPLYLN 8
            byte reply[GETTMNLV_RPLYLN] = { 0 };
            reply[0] = opCodeAck;
            reply[1] = ID_CHAR_1;                       /* N */
            reply[2] = ID_CHAR_2;                       /* B */
            reply[3] = ID_CHAR_3;                       /* T */
            reply[4] = TIMONEL_VER_MJR;                 /* Timonel Major version number */
            reply[5] = TIMONEL_VER_MNR;                 /* Timonel Minor version number */
            reply[6] = ((TIMONEL_START & 0xFF00) >> 8); /* Timonel Base Address MSB */
            reply[7] = (TIMONEL_START & 0xFF);          /* Timonel Base Address LSB */
            statusRegister |= (1 << ST_INIT_2);         /* Two-step init step 2: receive GETTMNLV command */
#if ENABLE_LED_UI               
            LED_UI_PORT &= ~(1 << LED_UI_PIN);          /* Two-step init: Turn led off to indicate correct initialization */
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
            byte reply[EXITTMNL_RPLYLN] = { 0 };
            reply[0] = opCodeAck;
            for (byte i = 0; i < EXITTMNL_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            statusRegister |= (1 << ST_EXIT_TML);
            break;
        }
        // ******************
        // * DELFLASH Reply *
        // ******************
        case DELFLASH: {
            #define DELFLASH_RPLYLN 1
            byte reply[DELFLASH_RPLYLN] = { 0 };
            reply[0] = opCodeAck;
            for (byte i = 0; i < DELFLASH_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
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
            flashPageAddr = ((command[1] << 8) + command[2]);   /* Sets the flash memory page base address */
            reply[0] = opCodeAck;
            reply[1] = (byte)(command[1] + command[2]);         /* Returns the sum of MSB and LSB of the page address */
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
            if ((flashPageAddr + pageIX) == 0) {
                appResetLSB = command[0];
                appResetMSB = command[1];
            }
            for (uint8_t i = 1; i < (RXDATASIZE + 1); i += 2) {
                boot_spm_busy_wait();
                boot_page_fill((flashPageAddr + pageIX), ((command[i + 1] << 8) | command[i]));
                reply[1] += (uint8_t)((command[i + 1]) + command[i]);   /* Reply checksum accumulator */
                pageIX += 2;
            }
            if (reply[1] != command[RXDATASIZE + 1]) {
                statusRegister &= ~(1 << ST_APP_READY);                 /* Payload received with errors, don't run it !!! */
                statusRegister |= (1 << ST_DEL_FLASH);                  /* Safety payload deletion ... */
            }
            for (uint8_t i = 0; i < WRITPAGE_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            break;
        }
        // ******************
        // * INITTINY Reply *
        // ******************
        case INITTINY: {
            #define INITTINY_RPLYLN 1
            byte reply[INITTINY_RPLYLN] = { 0 };
            reply[0] = opCodeAck;
            //statusRegister |= (1 << (ST_INIT_1 - 1)) + (1 << (ST_INIT_2 - 1)); /* Single-step init */
            statusRegister |= (1 << ST_INIT_1);                 /* Two-step init step 1: receive INITTINY command */
            for (byte i = 0; i < INITTINY_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
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

// Function DisableWatchDog
void DisableWatchDog(void) {
    MCUSR = 0;
    WDTCR = ((1 << WDCE) | (1 << WDE));
    WDTCR = ((1 << WDP2) | (1 << WDP1) | (1 << WDP0));
}

// Function SetCPUSpeed8MHz
void SetCPUSpeed8MHz(void) {
    cli();                          
    CLKPR = (1 << CLKPCE);          
    CLKPR = (0x00);                 
}

// Function DeleteFlash
void DeleteFlash(void) {
    word pageAddress = TIMONEL_START;
    while (pageAddress != RESET_PAGE) {
        pageAddress -= PAGE_SIZE;
        boot_page_erase(pageAddress);
    }
    flashPageAddr = pageAddress;
}

// Function CalculateTrampoline
void CalculateTrampoline(byte applJumpLowByte, byte applJumpHighByte) {
    word jumpOffset = (~(TIMONEL_START - (((((applJumpHighByte << 8) + applJumpLowByte) + 1) & 0x0FFF) << 1)) >> 1);
    jumpOffset++;
    tplJumpLowByte = (jumpOffset & 0xFF);
    tplJumpHighByte = (((jumpOffset & 0xF00) >> 8) + 0xC0);
}

// Function CreateTrampoline
void CreateTrampoline(void) {
    word address = (TIMONEL_START - PAGE_SIZE);
    for (int i = 0; i < PAGE_SIZE - 2; i += 2) {
        boot_spm_busy_wait();
        boot_page_fill(address + i, 0xffff);
    }
    boot_page_fill((TIMONEL_START - 2), (word)((word)(tplJumpHighByte << 8) + tplJumpLowByte));
    //boot_page_fill((TIMONEL_START - 2), 0xc2ef);
    boot_spm_busy_wait();
    boot_page_write(address);
}

// void CreateTrampoline(void) {
    // word address = (TIMONEL_START - PAGE_SIZE);
    // for (int i = 0; i < PAGE_SIZE - 2; i += 2) {
        // boot_spm_busy_wait();
        // boot_page_fill(address + i, 0xffff);
    // }
    // boot_page_fill((TIMONEL_START - 2), 0xCCDD);
    // boot_spm_busy_wait();
    // boot_page_write(address);
// }