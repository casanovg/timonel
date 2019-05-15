/*           _                         _
 *       _  (_)                       | |
 *     _| |_ _ ____   ___  ____  _____| |
 *    (_   _) |    \ / _ \|  _ \| ___ | |
 *      | |_| | | | | |_| | | | | ____| |
 *       \__)_|_|_|_|\___/|_| |_|_____)\_)
 *
 *  Timonel - TWI Bootloader for ATtiny85 MCUs
 *  Author: Gustavo Casanova
 *  ...........................................
 *  Version: 1.2 "Sandra" / 2019-04-28
 *  gustavo.casanova@nicebots.com
 */

/* Includes */
#include "timonel.h"

/* Please set the TWI address in Makefile.inc, the one here is a default value!
   ****************************************************************************
   TWI address range 08 to 35: Timonel bootloader
   TWI address range 36 to 63: Application firmware
   Each TWI node must have a unique bootloader address that corresponds
   to a defined application address, as shown in this table:
         -----------------------------------------------------------------------------------
   Boot: |08|09|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|
   Appl: |36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|
         -----------------------------------------------------------------------------------
*/
#if ((TWI_ADDR < 8) || (TWI_ADDR > 35))
    #pragma GCC warning "Timonel TWI address isn't defined or is out of range! Using default value: 8 (valid range: 8 to 35 decimal)"
    #undef TWI_ADDR
    #define TWI_ADDR    8                               /* Timonel TWI default address: 08 (0x08) */
#endif /* 8 <= TWI_ADDR <= 35 */

// This bootloader ...
#define TIMONEL_VER_MJR 1                               /* Timonel version major number   */
#define TIMONEL_VER_MNR 2                               /* Timonel version major number   */

// Configuration checks
#if (TIMONEL_START % PAGE_SIZE != 0)
    #error "TIMONEL_START in makefile must be a multiple of chip's pagesize"
#endif

#if (PAGE_SIZE > 64)
    #error "Timonel only supports pagesizes up to 64 bytes"
#endif

#if (!(AUTO_TPL_CALC) && !(CMD_STPGADDR))
    #error "If the AUTO_TPL_CALC option is disabled, then CMD_STPGADDR must be enabled in tml-config.h!"
#endif
                                
#if ((MST_PACKET_SIZE > (TWI_RX_BUFFER_SIZE / 2)) || ((SLV_PACKET_SIZE > (TWI_TX_BUFFER_SIZE / 2))))
    #pragma GCC warning "Don't set transmission data size too high to avoid affecting the TWI reliability!"
#endif

#if ((CYCLESTOEXIT > 0) && (CYCLESTOEXIT < 10))
    #pragma GCC warning "Do not set CYCLESTOEXIT too low, it could make difficult for TWI master to initialize on time!"
#endif

// Type definitions
typedef uint8_t byte;
typedef uint16_t word;
typedef void (* const fptr_t)(void);

// Global variables
byte command[(MST_DATA_SIZE * 2) + 2] = { 0 };          /* Command received from TWI master */
byte flags = 0;                                         /* Bit: 8,7,6,5: not used; 4: exit & run app; 3: delete app; 2, 1: initialized */
word page_addr = 0x0000;                                /* Flash memory page start address */
byte page_ix = 0;                                       /* Flash memory page index */
#if AUTO_TPL_CALC
byte app_reset_lsb = 0xFF;                              /* Application first byte  */
byte app_reset_msb = 0xFF;                              /* Application second byte*/
#endif /* AUTO_TPL_CALC */

// Jump to trampoline
static const fptr_t RunApplication = (const fptr_t)((TIMONEL_START - 2) / 2);
#if !(USE_WDT_RESET)
// Restart this bootloader
static const fptr_t RestartTimonel = (const fptr_t)(TIMONEL_START / 2);
#endif /* !USE_WDT_RESET */

// Prototypes
void ReceiveEvent(uint8_t);
void RequestEvent(void);

// Function Main
int main() {
    /*  ___________________
       |                   | 
       |    Setup Block    |
       |___________________|
    */
    MCUSR = 0;                                          /* Disable watchdog */
    WDTCR = ((1 << WDCE) | (1 << WDE));
    WDTCR = ((1 << WDP2) | (1 << WDP1) | (1 << WDP0));
    cli();                                              /* Disable Interrupts */
#if ENABLE_LED_UI
    LED_UI_DDR |= (1 << LED_UI_PIN);                    /* Set led pin data direction register for output */
#endif /* ENABLE_LED_UI */
#if !(MODE_16_MHZ)
    uint8_t factory_osccal = OSCCAL;                    /* Preserve factory oscillator calibration */
    OSCCAL += OSC_FAST;                                 /* With clock settings below 16MHz, speed up for better TWI performance */
#endif /* 16_MHZ_MODE */
#if SET_PRESCALER
    CLKPR = (1 << CLKPCE);                              /* Set the CPU prescaler division factor = 1 */
    CLKPR = (0x00);
#endif /* SET_PRESCALER */
    UsiTwiDriverInit();                                 /* Initialize TWI driver */
    fptrReceiveEvent = ReceiveEvent;                    /* Pointer to TWI receive event function */
    fptrRequestEvent = RequestEvent;                    /* Pointer to TWI request event function */
    __SPM_REG = (_BV(CTPB) | \
                 _BV(__SPM_ENABLE));                    /* Clear temporary page buffer */
    asm volatile("spm");
    uint8_t exit_delay = CYCLESTOEXIT;                  /* Delay to exit bootloader and run the application if not initialized */
    uint16_t led_delay = CYCLESTOBLINK;                 /* Delay for led blink */
    bool slow_ops_enabled = false;                      /* Run slow operations only after completing TWI handshake */
    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */
    for (;;) {
        /* ......................................................
           . TWI Interrupt Emulation .......................... .
           . Check the USI status register to verify whether    .
           . a TWI start condition handler should be triggered  .
           ......................................................
        */
        if (((USISR >> TWI_START_COND_FLAG) & true) && ((USICR >> TWI_START_COND_INT) & true)) {
            TwiStartHandler();                                  /* If so, run the USI start handler ... */
        }       
        /* ......................................................
           . TWI Interrupt Emulation .......................... .
           . Check the USI status register to verify whether a  .
           . 4-bit counter overflow handler should be triggered .
           ......................................................
        */
        if (((USISR >> USI_OVERFLOW_FLAG) & true) && ((USICR >> USI_OVERFLOW_INT) & true)) {
            slow_ops_enabled = UsiOverflowHandler();            /* If so, run the USI overflow handler ... */
        }
#if !(TWO_STEP_INIT)
        if (!((flags >> FL_INIT_1) & true)) {
#else
        if (!(((flags >> FL_INIT_1) & true) && ((flags >> FL_INIT_2) & true))) {
#endif /* TWO_STEP_INIT */
            // ===========================================
            // = Run this until is initialized by master =
            // ===========================================
            if (led_delay-- == 0) {
#if ENABLE_LED_UI               
                LED_UI_PORT ^= (1 << LED_UI_PIN);               /* Blinks on each main loop pass at CYCLESTOWAIT intervals */
#endif /* ENABLE_LED_UI */                
                if (exit_delay-- == 0) {
                    // ========================================
                    // = >>> Timeout: Run the application <<< =
                    // ========================================
#if !(MODE_16_MHZ)
                    OSCCAL = factory_osccal;                    /* Back the oscillator calibration to its original setting */
#endif /* 16_MHZ_MODE */
                    RunApplication();                           /* Count from CYCLESTOEXIT to 0, then exit to the application */
                }
            }
        } else {
            if (slow_ops_enabled == true) {
                slow_ops_enabled = false;
                // =======================================
                // = Exit bootloader and run application =
                // =======================================
                if ((flags >> FL_EXIT_TML) & true) {    
                    asm volatile("cbr r31, 0x80");              /* Clear bit 7 of r31 */
    #if !(MODE_16_MHZ)
                    OSCCAL = factory_osccal;                    /* Back the oscillator calibration to its original setting */
    #endif /* 16_MHZ_MODE */                    
                    RunApplication();                           /* Exit to the application */
                }
                // ========================================
                // = Delete application from flash memory =
                // ========================================
                if ((flags >> FL_DEL_FLASH) & true) {
    #if ENABLE_LED_UI                   
                    LED_UI_PORT |= (1 << LED_UI_PIN);           /* Turn led on to indicate erasing ... */
    #endif /* ENABLE_LED_UI */
                    word page_to_del = TIMONEL_START;           /* Erase flash ... */
                    while (page_to_del != RESET_PAGE) {
                        page_to_del -= PAGE_SIZE;
                        boot_page_erase(page_to_del);
                    }                    
    #if !(MODE_16_MHZ)
                    OSCCAL = factory_osccal;                    /* Back the oscillator calibration to its original setting */
    #endif /* 16_MHZ_MODE */                    
    #if !(USE_WDT_RESET)
                    RestartTimonel();                           /* Exit to the application, in this case restarts the bootloader */
    #else
                    wdt_enable(WDTO_15MS);                      /* RESETTING ... WARNING!!! */
                    for (;;) {};
    #endif /* !USE_WDT_RESET */                    
                }
                // ========================================================================
                // = Write received page to flash memory and prepare to receive a new one =
                // ========================================================================
    #if (APP_USE_TPL_PG || !(AUTO_TPL_CALC))
                if ((page_ix == PAGE_SIZE) & (page_addr < TIMONEL_START)) {
    #else
                if ((page_ix == PAGE_SIZE) & (page_addr < TIMONEL_START - PAGE_SIZE)) {
    #endif /* APP_USE_TPL_PG || !(AUTO_TPL_CALC) */
    #if ENABLE_LED_UI
                    LED_UI_PORT ^= (1 << LED_UI_PIN);           /* Turn led on and off to indicate writing ... */
    #endif /* ENABLE_LED_UI */
    #if FORCE_ERASE_PG
                    boot_page_erase(page_addr);
    #endif /* FORCE_ERASE_PG */                    
                    boot_page_write(page_addr);
    #if AUTO_TPL_CALC
                    if (page_addr == RESET_PAGE) {              /* Calculate and write trampoline */
                        word tpl = (((~((TIMONEL_START >> 1) - ((((app_reset_msb << 8) | app_reset_lsb) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
                        for (int i = 0; i < PAGE_SIZE - 2; i += 2) {
                            boot_page_fill((TIMONEL_START - PAGE_SIZE) + i, 0xFFFF);
                        }
                        boot_page_fill((TIMONEL_START - 2), tpl);
                        boot_page_write(TIMONEL_START - PAGE_SIZE);                        
                    }
    #if APP_USE_TPL_PG
                    if ((page_addr) == (TIMONEL_START - PAGE_SIZE)) {
                        word tpl = (((~((TIMONEL_START >> 1) - ((((app_reset_msb << 8) | app_reset_lsb) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
                        // - Read the previous page to the bootloader start, write it to the temporary buffer.
                        const __flash unsigned char * mem_addr;
                        for (byte i = 0; i < PAGE_SIZE - 2; i += 2) {
                            mem_addr = (void *)((TIMONEL_START - PAGE_SIZE) + i);
                            word page_data = (*mem_addr & 0xFF);
                            page_data += ((*(++mem_addr) & 0xFF) << 8); 
                            boot_page_fill((TIMONEL_START - PAGE_SIZE) + i, page_data);
                        }
                        // - Check if the last two bytes of the trampoline page are 0xFF.
                        mem_addr = (void *)(TIMONEL_START - 2);
                        word page_data = (*mem_addr & 0xFF);
                        page_data += ((*(++mem_addr) & 0xFF) << 8);
                        if (page_data == 0xFFFF) {
                            // -- If yes, then the application fits in memory, flash the trampoline bytes again.                            
                            boot_page_fill(TIMONEL_START - 2, tpl);
                            boot_page_erase(TIMONEL_START - PAGE_SIZE);
                            boot_page_write(TIMONEL_START - PAGE_SIZE);
                        } else {
                            // -- If no, it means that the application is too big for this setup, erase it! 
                            flags |= (1 << FL_DEL_FLASH);
                        }
                    }
    #endif /* APP_USE_TPL_PG */
    #endif /* AUTO_TPL_CALC */
    #if !(CMD_STPGADDR)
                    page_addr += PAGE_SIZE;
    #endif /* !CMD_STPGADDR */
                    page_ix = 0;
                }
            }
        }     
    }
    return(0);
}

/*  ________________________
   |                        |
   | TWI data receive event |
   |________________________|
*/
void ReceiveEvent(uint8_t received_bytes) {    
    for (uint8_t i = 0; i < received_bytes; i++) {
        command[i] = UsiTwiReceiveByte();                       /* Store the data sent by the TWI master in the data buffer */
    }
}

/*  ________________________
   |                        |
   | TWI data request event |
   |________________________|
*/
void RequestEvent(void) {    
    byte command_ack = ~command[0];                             /* Command code reply => ACK: command bitwise "not" */
    switch (command[0]) {
        // ******************
        // * GETTMNLV Reply *
        // ******************
        case GETTMNLV: {
            #define GETTMNLV_RPLYLN 10
            const __flash unsigned char * mem_addr;
            mem_addr = (void *)(TIMONEL_START - 1); 
            byte reply[GETTMNLV_RPLYLN] = { 0 };
            reply[0] = command_ack;
            reply[1] = ID_CHAR_3;                               /* T */            
            reply[2] = TIMONEL_VER_MJR;                         /* Major version number */
            reply[3] = TIMONEL_VER_MNR;                         /* Minor version number */
            reply[4] = TML_FEATURES;                            /* Optional features */
            reply[5] = ((TIMONEL_START & 0xFF00) >> 8);         /* Start address MSB */
            reply[6] = (TIMONEL_START & 0xFF);                  /* Start address LSB */
            reply[7] = (*mem_addr & 0xFF);                      /* Trampoline second byte (MSB) */
            reply[8] = (*(--mem_addr) & 0xFF);                  /* Trampoline first byte (LSB) */
#if CHECK_EMPTY_FL
            for (word mem_ix = 0; mem_ix < 100; mem_ix++) {     /* Check the first 100 memory positions to determine if  */
                mem_addr = (void *)(mem_ix);                    /* there is an application (or some other data) loaded.  */
                reply[9] += (byte)~(*mem_addr);                    
            }
#else
            reply[9] = OSCCAL;                                  /* Internal RC oscillator calibration */
#endif /* CHECK_EMPTY_FL */
            flags |= (1 << FL_INIT_1);                          /* First-step of single or two-step initialization */
#if ENABLE_LED_UI
            LED_UI_PORT &= ~(1 << LED_UI_PIN);                  /* Turn led off to indicate initialization */
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
            UsiTwiTransmitByte(command_ack);
            flags |= (1 << FL_EXIT_TML);
            break;
        }
        // ******************
        // * DELFLASH Reply *
        // ******************
        case DELFLASH: {
            UsiTwiTransmitByte(command_ack);
            flags |= (1 << FL_DEL_FLASH);
            break;
        }
#if (CMD_STPGADDR || !(AUTO_TPL_CALC))
        // ******************
        // * STPGADDR Reply *
        // ******************
        case STPGADDR: {
            #define STPGADDR_RPLYLN 2
            byte reply[STPGADDR_RPLYLN] = { 0 };
            page_addr = ((command[1] << 8) + command[2]);       /* Sets the flash memory page base address */
            page_addr &= ~(PAGE_SIZE - 1);                      /* Keep only pages' base addresses */
            reply[0] = command_ack;
            reply[1] = (byte)(command[1] + command[2]);         /* Returns the sum of MSB and LSB of the page address */
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
            reply[0] = command_ack;
            if ((page_addr + page_ix) == RESET_PAGE) {
#if AUTO_TPL_CALC
                app_reset_lsb = command[1];
                app_reset_msb = command[2];
#endif /* AUTO_TPL_CALC */
                // Modify the reset vector to point to this bootloader.
                // WARNING: This only works when CMD_STPGADDR is disabled. If CMD_STPGADDR is enabled,
                // the reset vector modification MUST BE done by the TWI master's upload program.
                // Otherwise, Timonel won't have the execution control after power on reset.
                boot_page_fill((RESET_PAGE), (0xC000 + ((TIMONEL_START / 2) - 1)));
                reply[1] += (byte)((command[2]) + command[1]);  /* Reply checksum accumulator */
                page_ix += 2;
                for (byte i = 3; i < (MST_DATA_SIZE + 1); i += 2) {
                    boot_page_fill((page_addr + page_ix), ((command[i + 1] << 8) | command[i]));
                    reply[1] += (byte)((command[i + 1]) + command[i]);
                    page_ix += 2;
                }                
            } else {
                for (byte i = 1; i < (MST_DATA_SIZE + 1); i += 2) {
                    boot_page_fill((page_addr + page_ix), ((command[i + 1] << 8) | command[i]));
                    reply[1] += (byte)((command[i + 1]) + command[i]);
                    page_ix += 2;
                }
            }
            if ((reply[1] != command[MST_DATA_SIZE + 1]) || (page_ix > PAGE_SIZE)) {
                flags |= (1 << FL_DEL_FLASH);                   /* If checksums don't match, safety payload deletion ... */
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
            const byte reply_len = (command[3] + 2);            /* Fourth byte received determines the size of reply data */
            byte reply[reply_len];
            if ((command[3] >= 1) & (command[3] <= SLV_DATA_SIZE) & ((byte)(command[0] + command[1] + command[2] + command[3]) == command[4])) {
                reply[0] = command_ack;
                page_addr = ((command[1] << 8) + command[2]);   /* Sets the flash memory page base address */
                reply[reply_len - 1] = 0;                       /* Checksum initialization */
                const __flash unsigned char * mem_addr;
                mem_addr = (void *)page_addr; 
                for (byte i = 1; i < command[3] + 1; i++) {
                    reply[i] = (*(mem_addr++) & 0xFF);          /* Actual flash data */
                    reply[reply_len - 1] += (byte)(reply[i]);   /* Checksum accumulator to be sent in the last byte of the reply */
                }                
                for (byte i = 0; i < reply_len; i++) {
                    UsiTwiTransmitByte(reply[i]);
                }
            } else {
                UsiTwiTransmitByte(UNKNOWNC);                   /* Incorrect operand value received */
            }
            break;
        }   
#endif /* CMD_READFLASH */
#if TWO_STEP_INIT
        // ******************
        // * INITSOFT Reply *
        // ******************
        case INITSOFT: {
            flags |= (1 << FL_INIT_2);                          /* Second-step of two-step initialization */
            UsiTwiTransmitByte(command_ack);
            break;
        }
#endif /* TWO_STEP_INIT */
        default: {
            UsiTwiTransmitByte(UNKNOWNC);
            break;
        }
    }
}
