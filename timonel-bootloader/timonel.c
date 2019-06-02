/*           _                         _
 *       _  (_)                       | |
 *     _| |_ _ ____   ___  ____  _____| |
 *    (_   _) |    \ / _ \|  _ \| ___ | |
 *      | |_| | | | | |_| | | | | ____| |
 *       \__)_|_|_|_|\___/|_| |_|_____)\_)
 *
 *  Timonel - TWI Bootloader for TinyX5 MCUs
 *  Author: Gustavo Casanova
 *  ...........................................
 *  Version: 1.3 "Sandra" / 2019-06-06
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
#pragma GCC warning "Timonel TWI address isn't defined or is out of range! Using default value: 11 (valid range: 8 to 35 decimal)"
#undef TWI_ADDR
#define TWI_ADDR   11                                   /* Timonel TWI default address: 11 (0x0B) */
#endif /* 8 <= TWI_ADDR <= 35 */

/* This bootloader ... */
#define TIMONEL_VER_MJR 1                               /* Timonel version major number   */
#define TIMONEL_VER_MNR 3                               /* Timonel version major number   */

/* Configuration checks */
#if (TIMONEL_START % SPM_PAGESIZE != 0)
#error "TIMONEL_START in makefile must be a multiple of chip's pagesize"
#endif

#if (SPM_PAGESIZE > 64)
#error "Timonel only supports pagesizes up to 64 bytes"
#endif

#if (!(AUTO_PAGE_ADDR) && !(CMD_SETPGADDR))
#error "If the AUTO_PAGE_ADDR option is disabled, then CMD_SETPGADDR must be enabled in tml-config.h!"
#endif
                                
#if ((MST_PACKET_SIZE > (TWI_RX_BUFFER_SIZE / 2)) || ((SLV_PACKET_SIZE > (TWI_TX_BUFFER_SIZE / 2))))
#pragma GCC warning "Don't set transmission data size too high to avoid affecting the TWI reliability!"
#endif

#if ((CYCLESTOEXIT > 0) && (CYCLESTOEXIT < 10))
#pragma GCC warning "Do not set CYCLESTOEXIT too low, it could make difficult for TWI master to initialize on time!"
#endif

// Type definitions
typedef void (* const fptr_t)(void);                    /* Pointer-to-function type */
typedef enum {                                          /* TWI driver operational modes */
    STATE_CHECK_RECEIVED_ADDRESS,
    STATE_SEND_DATA_BYTE,
    STATE_RECEIVE_ACK_AFTER_SENDING_DATA,
    STATE_CHECK_RECEIVED_ACK,
    STATE_RECEIVE_DATA_BYTE,
    STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK
} OverflowState;

// Bootloader globals
uint8_t flags = 0;                                      /* Bit: 8, 7, 6, 5: not used; 4: exit; 3: delete app; 2, 1: initialized */
uint8_t page_ix = 0;                                    /* Flash memory page index */
uint16_t page_addr = 0x0000;                            /* Flash memory page address */
#if AUTO_PAGE_ADDR
uint8_t app_reset_lsb = 0xFF;                           /* Application first byte: reset vector LSB */
uint8_t app_reset_msb = 0xFF;                           /* Application second byte: reset vector MSB */
#endif /* AUTO_PAGE_ADDR */
static const fptr_t RunApplication = (const fptr_t)((TIMONEL_START - 2) / 2); /* Pointer to trampoline to app address */
#if !(USE_WDT_RESET)
static const fptr_t RestartTimonel = (const fptr_t)(TIMONEL_START / 2);       /* Pointer to bootloader start address */
#endif /* !USE_WDT_RESET */

// USI TWI driver globals
uint8_t rx_buffer[TWI_RX_BUFFER_SIZE];
uint8_t tx_buffer[TWI_TX_BUFFER_SIZE];
uint8_t rx_byte_count = 0;                              /* Bytes received in RX buffer */
uint8_t rx_head = 0, rx_tail = 0;
uint8_t tx_head = 0, tx_tail = 0;
OverflowState device_state;

// Bootloader prototypes
inline static void ReceiveEvent(uint8_t) __attribute__((always_inline));
inline static void ResetPrescaler(void) __attribute__((always_inline));
inline static void RestorePrescaler(void) __attribute__((always_inline));

// USI TWI driver prototypes
void UsiTwiTransmitByte(uint8_t);
uint8_t UsiTwiReceiveByte(void);
inline static void UsiTwiDriverInit(void) __attribute__((always_inline));
inline static void TwiStartHandler(void) __attribute__((always_inline));
inline static bool UsiOverflowHandler(void) __attribute__((always_inline));

// USI TWI driver basic operations prototypes
void SET_USI_TO_WAIT_FOR_TWI_ADDRESS(void);
inline static void SET_USI_TO_SEND_BYTE(void) __attribute__((always_inline));
inline static void SET_USI_TO_RECEIVE_BYTE(void) __attribute__((always_inline));
inline static void SET_USI_TO_SEND_ACK(void) __attribute__((always_inline));
inline static void SET_USI_TO_RECEIVE_ACK(void) __attribute__((always_inline));
inline static void SET_USI_TO_DETECT_TWI_START(void) __attribute__((always_inline));
inline static void SET_USI_TO_DETECT_TWI_RESTART(void) __attribute__((always_inline));
inline static void SET_USI_TO_SHIFT_8_ADDRESS_BITS(void) __attribute__((always_inline));
inline static void SET_USI_TO_SHIFT_8_DATA_BITS(void) __attribute__((always_inline));
inline static void SET_USI_TO_SHIFT_1_ACK_BIT(void) __attribute__((always_inline));

// USI TWI driver direction setting prototypes
inline static void SET_USI_SDA_AS_OUTPUT(void) __attribute__((always_inline));
inline static void SET_USI_SDA_AS_INPUT(void) __attribute__((always_inline));
inline static void SET_USI_SCL_AS_OUTPUT(void) __attribute__((always_inline));
inline static void SET_USI_SCL_AS_INPUT(void) __attribute__((always_inline));
inline static void SET_USI_SDA_AND_SCL_AS_OUTPUT(void) __attribute__((always_inline));
inline static void SET_USI_SDA_AND_SCL_AS_INPUT(void) __attribute__((always_inline));

// Main function
int main(void) {
    /*  ___________________
       |                   | 
       |    Setup Block    |
       |___________________|
    */
    MCUSR = 0;                                          /* Disable watchdog */
    WDTCR = ((1 << WDCE) | (1 << WDE));
    WDTCR = ((1 << WDP2) | (1 << WDP1) | (1 << WDP0));
    cli();                                              /* Disable interrupts */
#if ENABLE_LED_UI
    LED_UI_DDR |= (1 << LED_UI_PIN);                    /* Set led pin data direction register for output */
#endif /* ENABLE_LED_UI */
    uint8_t exit_delay = SHORT_EXIT_DLY;                /* Exit-to-app delay when the bootloader isn't initialized */                           
    uint16_t led_delay = SHORT_LED_DLY;                 /* Blinking delay when the bootloader isn't initialized */
#if AUTO_CLK_TWEAK                                      /* Automatic clock tweaking made at run time, based on low fuse value */
#pragma message "AUTO CLOCK TWEAKING SELECTED: Clock adjustments will be made at run time ..."
    uint8_t factory_osccal = OSCCAL;                    /* Preserve factory oscillator calibration */
    if ((boot_lock_fuse_bits_get(L_FUSE_ADDR) & 0x0F) == RCOSC_CLK_SRC) {
        // RC oscillator (8 MHz) clock source set in low fuse, calibrating oscillator up ...
        OSCCAL += OSC_FAST;                             /* Speed oscillator up for TWI to work */
    } else if ((boot_lock_fuse_bits_get(L_FUSE_ADDR) & 0x0F) == HFPLL_CLK_SRC) {
        // HF PLL (16 MHz) clock source set in low fuse. No clock tweaking needed ...
    } else {
        // Unknown clock source set in low fuse! the prescaler will be reset to 1 to use the external clock as is
        ResetPrescaler();                               /* If using an external CPU clock source, don't reduce its frequency */
    }
    if (!((boot_lock_fuse_bits_get(L_FUSE_ADDR) >> LFUSE_PRESC_BIT) & true)) {
        // Prescaler fuse bit set to divide clock by 8, setting the CPU prescaler division factor to 1
        ResetPrescaler();                               /* Reset prescaler to divide by 1 */
    }
#else                                                   /* Clock tweaking made at compile time, based on LOW_FUSE variable */
#define XSTR(x) STR(x)
#define STR(x) #x
#pragma message "CLOCK TWEAKING AT COMPILE TIME BASED ON LOW_FUSE VARIABLE: " XSTR(LOW_FUSE)
#if ((LOW_FUSE & 0x0F) == RCOSC_CLK_SRC)                /* RC oscillator (8 MHz) clock source */
#pragma message "RC oscillator (8 MHz) clock source selected, calibrating oscillator up ..."
    uint8_t factory_osccal = OSCCAL;                    /* With 8 MHz clock source, preserve factory oscillator  */
    OSCCAL += OSC_FAST;                                 /* calibration and speed it up for TWI to work.          */
#elif ((LOW_FUSE & 0x0F) == HFPLL_CLK_SRC)              /* HF PLL (16 MHz) clock source */
#pragma message "HF PLL (16 MHz) clock source selected. No clock tweaking needed ..."
#else                                                   /* Unknown clock source */
#pragma GCC warning "UNKNOWN LOW_FUSE CLOCK SETTING! VALID VALUES ARE 0xE1, 0x61, 0xE2 and 0x62"
    ResetPrescaler();                                   /* If using an external CPU clock source, don't reduce its frequency */
#endif /* LOW_FUSE CLOCK SOURCE */
#if ((LOW_FUSE & 0x80) == 0)                            /* Prescaler dividing clock by 8 */                      
#pragma message "Prescaler dividing clock by 8, setting the CPU prescaler division factor to 1 ..."
    ResetPrescaler();                                   /* Reset prescaler to divide by 1 */
#endif /* LOW_FUSE PRESCALER BIT */
#endif /* AUTO_CLK_TWEAK */
    UsiTwiDriverInit();                                 /* Initialize the TWI driver */
    __SPM_REG = (_BV(CTPB) | _BV(__SPM_ENABLE));        /* Prepare to clear the temporary page buffer */                 
    asm volatile("spm");                                /* Run SPM instruction to complete the clearing */
    bool slow_ops_enabled = false;                      /* Allow slow operations only after completing TWI handshake */
    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */
    for (;;) {
        /* ......................................................
           . TWI Interrupt Emulation >>>>>>>>>>>>>>>>>>>>>>>>>>  .
           . Check the USI status register to verify whether      .
           . a TWI start condition handler should be triggered   .
           ......................................................
        */
        if (((USISR >> TWI_START_COND_FLAG) & true) && ((USICR >> TWI_START_COND_INT) & true)) {
            TwiStartHandler();                          /* If so, run the USI start handler ... */
        }       
        /* ......................................................
           . TWI Interrupt Emulation >>>>>>>>>>>>>>>>>>>>>>>>>>  .
           . Check the USI status register to verify whether a    .
           . 4-bit counter overflow handler should be triggered  .
           ......................................................
        */
        if (((USISR >> USI_OVERFLOW_FLAG) & true) && ((USICR >> USI_OVERFLOW_INT) & true)) {
            slow_ops_enabled = UsiOverflowHandler();    /* If so, run the USI overflow handler ... */
        }
#if !(TWO_STEP_INIT)
        if ((flags >> FL_INIT_1) & true) {
#else
        if (((flags >> FL_INIT_1) & true) && ((flags >> FL_INIT_2) & true)) {
#endif /* TWO_STEP_INIT */
            // ======================================
            // =   *\* Bootloader initialized */*   =
            // ======================================           
            if (slow_ops_enabled == true) {
                slow_ops_enabled = false;
                // =======================================================
                // = Exit the bootloader & run the application (Slow Op) =
                // =======================================================
                if ((flags >> FL_EXIT_TML) & true) {
#if CLEAR_BIT_7_R31
                    asm volatile("cbr r31, 0x80");      /* Clear bit 7 of r31 */
#endif /* CLEAR_BIT_7_R31 */
#if AUTO_CLK_TWEAK
                    if ((boot_lock_fuse_bits_get(L_FUSE_ADDR) & 0x0F) == RCOSC_CLK_SRC) {
                        OSCCAL = factory_osccal;        /* Back the oscillator calibration to its original setting */
                    }
                    if (!((boot_lock_fuse_bits_get(L_FUSE_ADDR) >> LFUSE_PRESC_BIT) & true)) {
                        RestorePrescaler();             /* Restore prescaler to divide by 8 */
                    }                    
#else
#if ((LOW_FUSE & 0x0F) == RCOSC_CLK_SRC)
                    OSCCAL = factory_osccal;            /* Back the oscillator calibration to its original setting */
#endif /* LOW_FUSE RC OSC */
#if ((LOW_FUSE & 0x80) == 0)                            /* Prescaler dividing clock by 8 */                      
                    RestorePrescaler();                 /* Restore prescaler factor to divide by 8 */
#endif /* PRESCALER BIT */
#endif /* AUTO_CLK_TWEAK */
                    RunApplication();                   /* Exit to the application */
                }
                // ================================================
                // = Delete the application from memory (Slow Op) =
                // ================================================
                if ((flags >> FL_DEL_FLASH) & true) {
#if ENABLE_LED_UI                   
                    LED_UI_PORT |= (1 << LED_UI_PIN);   /* Turn led on to indicate erasing ... */
#endif /* ENABLE_LED_UI */
                    uint16_t page_to_del = TIMONEL_START;
                    while (page_to_del != RESET_PAGE) {
                        page_to_del -= SPM_PAGESIZE;
                        boot_page_erase(page_to_del);   /* Erase flash memory ... */
                    }
#if AUTO_CLK_TWEAK
                    if ((boot_lock_fuse_bits_get(L_FUSE_ADDR) & 0x0F) == RCOSC_CLK_SRC) {
                        OSCCAL = factory_osccal;        /* Back the oscillator calibration to its original setting */
                    }
#else
#if ((LOW_FUSE & 0x0F) == RCOSC_CLK_SRC)
                    OSCCAL = factory_osccal;            /* Back the oscillator calibration to its original setting */
#endif /* LOW_FUSE RC OSC */
#endif /* AUTO_CLK_TWEAK */
#if !(USE_WDT_RESET)
                    RestartTimonel();                   /* Restart by jumping to Timonel start */
#else
                    wdt_enable(WDTO_15MS);              /* Restart by activating the watchdog timer */
                    for (;;) {};
#endif /* !USE_WDT_RESET */
                }
                // =========================================================================
                // = Write the received page to memory and prepare for a new one (Slow Op) =
                // =========================================================================
#if (APP_USE_TPL_PG || !(AUTO_PAGE_ADDR))                
                if ((page_ix == SPM_PAGESIZE) && (page_addr < TIMONEL_START)) {
#else
                if ((page_ix == SPM_PAGESIZE) && (page_addr < TIMONEL_START - SPM_PAGESIZE)) {
#endif /* APP_USE_TPL_PG || !(AUTO_PAGE_ADDR) */
#if ENABLE_LED_UI
                    LED_UI_PORT ^= (1 << LED_UI_PIN);   /* Turn led on and off to indicate writing ... */
#endif /* ENABLE_LED_UI */
#if FORCE_ERASE_PG
                    boot_page_erase(page_addr);
#endif /* FORCE_ERASE_PG */                    
                    boot_page_write(page_addr);
#if AUTO_PAGE_ADDR
                    if (page_addr == RESET_PAGE) {      /* Calculate and write trampoline */
                        uint16_t tpl = (((~((TIMONEL_START >> 1) - ((((app_reset_msb << 8) | app_reset_lsb) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
                        for (int i = 0; i < SPM_PAGESIZE - 2; i += 2) {
                            boot_page_fill((TIMONEL_START - SPM_PAGESIZE) + i, 0xFFFF);
                        }
                        boot_page_fill((TIMONEL_START - 2), tpl);
                        boot_page_write(TIMONEL_START - SPM_PAGESIZE);                        
                    }
#if APP_USE_TPL_PG
                    if (page_addr == (TIMONEL_START - SPM_PAGESIZE)) {
                        uint16_t tpl = (((~((TIMONEL_START >> 1) - ((((app_reset_msb << 8) | app_reset_lsb) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
                        // - Read the previous page to the bootloader start, write it to the temporary buffer.
                        const __flash uint8_t *mem_position;
                        for (uint8_t i = 0; i < SPM_PAGESIZE - 2; i += 2) {
                            mem_position = (void *)((TIMONEL_START - SPM_PAGESIZE) + i);
                            uint16_t page_data = (*mem_position & 0xFF);
                            page_data += ((*(++mem_position) & 0xFF) << 8); 
                            boot_page_fill((TIMONEL_START - SPM_PAGESIZE) + i, page_data);
                        }
                        // - Check if the last two bytes of the trampoline page are 0xFF.
                        mem_position = (void *)(TIMONEL_START - 2);
                        uint16_t page_data = (*mem_position & 0xFF);
                        page_data += ((*(++mem_position) & 0xFF) << 8);
                        if (page_data == 0xFFFF) {
                            // -- If yes, then the application fits in memory, flash the trampoline bytes again.                            
                            boot_page_fill(TIMONEL_START - 2, tpl);
                            boot_page_erase(TIMONEL_START - SPM_PAGESIZE);
                            boot_page_write(TIMONEL_START - SPM_PAGESIZE);
                        } else {
                            // -- If no, it means that the application is too big for this setup, erase it! 
                            flags |= (1 << FL_DEL_FLASH);
                        }
                    }
#endif /* APP_USE_TPL_PG */
                    page_addr += SPM_PAGESIZE;
#endif /* AUTO_PAGE_ADDR */
                    page_ix = 0;
				}
            }
        } else {
            // ======================================
            // = *\* Bootloader not initialized */* =
            // ======================================
            if (led_delay-- == 0) {
#if ENABLE_LED_UI               
                LED_UI_PORT ^= (1 << LED_UI_PIN);       /* If Timonel isn't initialized, led blinks at LED_DLY intervals */
#endif /* ENABLE_LED_UI */                
                if (exit_delay-- == 0) {
                    // ========================================
                    // = >>> Timeout: Run the application <<< =
                    // ========================================
#if AUTO_CLK_TWEAK
                    if ((boot_lock_fuse_bits_get(L_FUSE_ADDR) & 0x0F) == RCOSC_CLK_SRC) {
                        OSCCAL = factory_osccal;        /* Back the oscillator calibration to its original setting */
                    }
                    if (!((boot_lock_fuse_bits_get(L_FUSE_ADDR) >> LFUSE_PRESC_BIT) & true)) {
                        RestorePrescaler();             /* Restore prescaler to divide by 8 */
                    }
#else
#if ((LOW_FUSE & 0x0F) == RCOSC_CLK_SRC)
                    OSCCAL = factory_osccal;            /* Back the oscillator calibration to its original setting */
#endif /* LOW_FUSE RC OSC */
#if ((LOW_FUSE & 0x80) == 0)                            /* Prescaler dividing clock by 8 */                      
                    RestorePrescaler();                 /* Restore prescaler factor to divide by 8 */
#endif /* PRESCALER BIT */ 
#endif /* AUTO_CLK_TWEAK */
                    RunApplication();                   /* Count from CYCLESTOEXIT to 0, then exit to the application */
                }
            }
        }
    }
    return 0;
}

/*  ________________________
   |                        |
   | TWI data receive event |
   |________________________|
*/
inline void ReceiveEvent(uint8_t received_bytes) {
    // Read the received bytes from RX buffer
    static uint8_t command[MST_PACKET_SIZE * 2];
    for (uint8_t i = 0; i < received_bytes; i++) {
        command[i] = UsiTwiReceiveByte();
    }
    // If there is a valid bootloader command, execute it
    switch (command[0]) {
        // ******************
        // * GETTMNLV Reply *
        // ******************
        case GETTMNLV: {
            const __flash uint8_t *mem_position;
            mem_position = (void *)(TIMONEL_START - 1); 
            uint8_t reply[GETTMNLV_RPLYLN];
            reply[0] = ACKTMNLV;
            reply[1] = ID_CHAR_3;                       /* "T" Signature */
            reply[2] = TIMONEL_VER_MJR;                 /* Major version number */
            reply[3] = TIMONEL_VER_MNR;                 /* Minor version number */
            reply[4] = TML_FEATURES;                    /* Optional features */
            reply[5] = ((TIMONEL_START & 0xFF00) >> 8); /* Start address MSB */
            reply[6] = (TIMONEL_START & 0xFF);          /* Start address LSB */
            reply[7] = *mem_position;                   /* Trampoline second byte (MSB) */
            reply[8] = *(--mem_position);               /* Trampoline first byte (LSB) */
            reply[9] = boot_lock_fuse_bits_get(0);      /* Low fuse setting */
#if CHECK_EMPTY_FL
            // Check the first 100 memory positions to determine if
            // there is an application (or some other data) loaded.
            for (uint16_t mem_ix = 0; mem_ix < 100; mem_ix++) {
                mem_position = (void *)(mem_ix);
                reply[10] += (uint8_t)~(*mem_position);                    
            }
#else
            reply[10] = OSCCAL;                         /* Internal RC oscillator calibration */
#endif /* CHECK_EMPTY_FL */
            flags |= (1 << FL_INIT_1);                  /* First-step of single or two-step initialization */
#if ENABLE_LED_UI
            LED_UI_PORT &= ~(1 << LED_UI_PIN);          /* Turn led off to indicate initialization */
#endif /* ENABLE_LED_UI */
            for (uint8_t i = 0; i < GETTMNLV_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            return;
        }
        // ******************
        // * EXITTMNL Reply *
        // ******************
        case EXITTMNL: {
            UsiTwiTransmitByte(ACKEXITT);
            flags |= (1 << FL_EXIT_TML);
            return;
        }
        // ******************
        // * DELFLASH Reply *
        // ******************
        case DELFLASH: {
            UsiTwiTransmitByte(ACKDELFL);
            flags |= (1 << FL_DEL_FLASH);
            return;
        }
#if (CMD_SETPGADDR || !(AUTO_PAGE_ADDR))
        // ******************
        // * STPGADDR Reply *
        // ******************
        case STPGADDR: {
            uint8_t reply[STPGADDR_RPLYLN] = { 0 };
            page_addr = ((command[1] << 8) + command[2]);   /* Sets the flash memory page base address */
            page_addr &= ~(SPM_PAGESIZE - 1);               /* Keep only pages' base addresses */
            reply[0] = AKPGADDR;
            reply[1] = (uint8_t)(command[1] + command[2]);  /* Returns the sum of MSB and LSB of the page address */
            for (uint8_t i = 0; i < STPGADDR_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            return;
        }
#endif /* CMD_SETPGADDR || !AUTO_PAGE_ADDR */
        // ******************
        // * WRITPAGE Reply *
        // ******************
        case WRITPAGE: {
            uint8_t reply[WRITPAGE_RPLYLN] = { 0 };
            reply[0] = ACKWTPAG;
            if ((page_addr + page_ix) == RESET_PAGE) {
#if AUTO_PAGE_ADDR
                app_reset_lsb = command[1];
                app_reset_msb = command[2];
#endif /* AUTO_PAGE_ADDR */
                // This section modifies the reset vector to point to this bootloader.
                // WARNING: This only works when CMD_SETPGADDR is disabled. If CMD_SETPGADDR is enabled,
                // the reset vector modification MUST BE done by the TWI master's upload program.
                // Otherwise, Timonel won't have the execution control after power-on reset.
                boot_page_fill((RESET_PAGE), (0xC000 + ((TIMONEL_START / 2) - 1)));
                reply[1] += (uint8_t)((command[2]) + command[1]);   /* Reply checksum accumulator */
                page_ix += 2;
                for (uint8_t i = 3; i < (MST_PACKET_SIZE + 1); i += 2) {
                    boot_page_fill((page_addr + page_ix), ((command[i + 1] << 8) | command[i]));
                    reply[1] += (uint8_t)((command[i + 1]) + command[i]);
                    page_ix += 2;
                }                
            } else {
                for (uint8_t i = 1; i < (MST_PACKET_SIZE + 1); i += 2) {
                    boot_page_fill((page_addr + page_ix), ((command[i + 1] << 8) | command[i]));
                    reply[1] += (uint8_t)((command[i + 1]) + command[i]);
                    page_ix += 2;
                }
            }
#if CHECK_PAGE_IX
            if ((reply[1] != command[MST_PACKET_SIZE + 1]) || (page_ix > SPM_PAGESIZE)) {
#else
            if (reply[1] != command[MST_PACKET_SIZE + 1]) {
#endif /* CHECK_PAGE_IX */
                flags |= (1 << FL_DEL_FLASH);                       /* If checksums don't match, safety payload deletion ... */
                reply[1] = 0;
            }
            for (uint8_t i = 0; i < WRITPAGE_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            return;
        }
#if CMD_READFLASH
        // ******************
        // * READFLSH Reply *
        // ******************
        case READFLSH: {
            const uint8_t reply_len = (command[3] + 2);             /* Reply length: ack + memory positions requested + checksum */
            uint8_t reply[reply_len];
            reply[0] = ACKRDFSH;
            reply[reply_len - 1] = 0;                               /* Checksum initialization */
            // Point the initial memory position to the received address, then
            // advance to fill the reply with the requested data amount.
            const __flash uint8_t *mem_position;
            mem_position = (void *)((command[1] << 8) + command[2]);
            for (uint8_t i = 1; i < command[3] + 1; i++) {
                reply[i] = (*(mem_position++) & 0xFF);              /* Actual memory position data */
                reply[reply_len - 1] += (uint8_t)(reply[i]);        /* Checksum accumulator */
            }
            reply[reply_len - 1] += (uint8_t)(command[1]);          /* Add Received address MSB to checksum */
            reply[reply_len - 1] += (uint8_t)(command[2]);          /* Add Received address MSB to checksum */
            for (uint8_t i = 0; i < reply_len; i++) {
                UsiTwiTransmitByte(reply[i]);                   
            }
#if ENABLE_LED_UI               
            LED_UI_PORT ^= (1 << LED_UI_PIN);                       /* Blinks whenever a memory data block is sent */
#endif /* ENABLE_LED_UI */          
            return;
        }        
#endif /* CMD_READFLASH */
#if TWO_STEP_INIT
        // ******************
        // * INITSOFT Reply *
        // ******************
        case INITSOFT: {
            flags |= (1 << FL_INIT_2);                              /* Two-step init step 1: receive INITSOFT command */
            UsiTwiTransmitByte(ACKINITS);
            return;
        }
#endif /* TWO_STEP_INIT */
        default: {
            UsiTwiTransmitByte(UNKNOWNC);                           /* Command not recognized */
            return;
        }
    }
}

// Function ResetPrescaler
inline void ResetPrescaler(void) {
    // Set the CPU prescaler division factor to 1
    CLKPR = (1 << CLKPCE);                                          /* Prescaler enable */
    CLKPR = 0x00;                                                   /* Clock division factor 1 (0000) */
}

// Function RestorePrescaler
inline void RestorePrescaler(void) {
    // Set the CPU prescaler division factor to 8
    CLKPR = (1 << CLKPCE);                                          /* Prescaler enable */
    CLKPR = ((1 << CLKPS1) | (1 << CLKPS0));                        /* Clock division factor 8 (0011) */
}

/////////////////////////////////////////////////////////////////////////////
////////////       ALL USI TWI DRIVER CODE BELOW THIS LINE       ////////////
/////////////////////////////////////////////////////////////////////////////

/*  ___________________________
   |                           |
   | USI TWI byte transmission |
   |___________________________|
*/
void UsiTwiTransmitByte(uint8_t data_byte) {
    tx_head = ((tx_head + 1) & TWI_TX_BUFFER_MASK); /* Update the TX buffer index */
    while (tx_head == tx_tail) {};          /* Wait until there is free space in the TX buffer */
    tx_buffer[tx_head] = data_byte;         /* Write the data byte into the TX buffer */
}

/*  ___________________________
   |                           |
   | USI TWI byte reception    |
   |___________________________|
*/
uint8_t UsiTwiReceiveByte(void) {
    while (rx_byte_count-- == 0) {};        /* Wait until a byte is received into the RX buffer */
    rx_tail = ((rx_tail + 1) & TWI_RX_BUFFER_MASK); /* Update the RX buffer index */
    return rx_buffer[rx_tail];              /* Return data from the buffer */
}

/*  _______________________________
   |                               |
   | USI TWI driver initialization |
   |_______________________________|
*/
void UsiTwiDriverInit(void) {
    // Initialize USI for TWI Slave mode.
    tx_tail = tx_head = 0;                  /* Flush TWI TX buffers */
    rx_tail = rx_head = rx_byte_count = 0;  /* Flush TWI RX buffers */
    SET_USI_SDA_AND_SCL_AS_OUTPUT();        /* Set SCL and SDA as output */
    PORT_USI |= (1 << PORT_USI_SDA);        /* Set SDA high */
    PORT_USI |= (1 << PORT_USI_SCL);        /* Set SCL high */
    SET_USI_SDA_AS_INPUT();                 /* Set SDA as input */
    SET_USI_TO_WAIT_FOR_TWI_ADDRESS();      /* Wait for TWI start condition and address from master */
}

/*  _______________________________________________________
   |                                                       |
   | TWI start condition handler (Interrupt-like function) |
   |_______________________________________________________|
*/
inline void TwiStartHandler(void) {
    SET_USI_SDA_AS_INPUT();                 /* Float the SDA line */
    // Following a start condition, the device shifts the address present on the TWI bus in and
    // a 4-bit counter overflow is triggered. Afterward, within the overflow handler, the device
    // should check whether it has to reply. Prepare the next overflow handler state for it.    
    // Next state -> STATE_CHECK_RECEIVED_ADDRESS
    device_state = STATE_CHECK_RECEIVED_ADDRESS;
    while ((PIN_USI & (1 << PORT_USI_SCL)) && (!(PIN_USI & (1 << PORT_USI_SDA)))) {
        // Wait for SCL to go low to ensure the start condition has completed.
        // The start detector will hold SCL low.
    }
    // If a stop condition arises then leave this function to prevent waiting forever.
    // Don't use USISR to test for stop condition as in application note AVR312
    // because the stop condition flag is going to be set from the last TWI sequence.
    if (!(PIN_USI & (1 << PIN_USI_SDA))) {
        // ==> Stop condition NOT DETECTED
        SET_USI_TO_DETECT_TWI_RESTART();
    } else {
        // ==> Stop condition DETECTED
        SET_USI_TO_DETECT_TWI_START();
    }
    // Read the address present on the TWI bus
    SET_USI_TO_SHIFT_8_ADDRESS_BITS();
}

/*  ______________________________________________________
   |                                                      |
   | USI 4-bit overflow handler (Interrupt-like function) |
   |______________________________________________________|
*/
inline bool UsiOverflowHandler(void) {    
    switch (device_state) {
        // If the address received after the start condition matches this device or is
        // a general call, reply ACK and check whether it should send or receive data.
        // Otherwise, set USI to wait for the next start condition and address.
        case STATE_CHECK_RECEIVED_ADDRESS: {
            if ((USIDR == 0) || ((USIDR >> 1) == TWI_ADDR)) {
                if (USIDR & 0x01) {     /* If data register low-order bit = 1, start the send data mode */
                    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                    //                                                                   >>
                    ReceiveEvent(rx_byte_count); // Call a main function to process data   >>
                    //                                                                   >>
                    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                    // Next state -> STATE_SEND_DATA_BYTE
                    device_state = STATE_SEND_DATA_BYTE;
                } else {                /* If data register low-order bit = 0, start the receive data mode */
                    // Next state -> STATE_RECEIVE_DATA_BYTE
                    device_state = STATE_RECEIVE_DATA_BYTE;
                }
                SET_USI_TO_SEND_ACK();
            } else {
                SET_USI_TO_WAIT_FOR_TWI_ADDRESS();
            }
            return false;
        }
        // Send data mode:
        //================
        // 3) Check whether the acknowledge bit received from the master is ACK or
        // NACK. If ACK (low), just continue to STATE_SEND_DATA_BYTE without break. If NACK (high)
        // the transmission is complete. Wait for a new start condition and TWI address.        
        case STATE_CHECK_RECEIVED_ACK: {
            if (USIDR) {                /* NACK - handshake complete ... */
                SET_USI_TO_WAIT_FOR_TWI_ADDRESS();
                // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                //                                                                   >>
                return true; // Enable slow operations in main!                        >>
                //                                                                   >> 
                // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
             }
             // Just drop straight into STATE_SEND_DATA_BYTE (no break) ...
        }
        // 1) Copy data from TX buffer to USIDR and set USI to shift 8 bits out. When the 4-bit
        // counter overflows, it means that a byte has been transmitted, so this device is ready
        // to transmit again or wait for a new start condition and address on the bus.
        case STATE_SEND_DATA_BYTE: {
            if (tx_head != tx_tail) {
                // If the TX buffer has data, copy the next byte to USI data register for sending                
                tx_tail = ((tx_tail + 1) & TWI_TX_BUFFER_MASK);
                USIDR = tx_buffer[tx_tail];
            } else {
                // If the buffer is empty ...
                SET_USI_TO_RECEIVE_ACK();  /* This might be necessary (http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&p=805227#805227) */
                SET_USI_TO_WAIT_FOR_TWI_ADDRESS();
                return false;
            }
            // Next state -> STATE_RECEIVE_ACK_AFTER_SENDING_DATA 
            device_state = STATE_RECEIVE_ACK_AFTER_SENDING_DATA; 
            SET_USI_TO_SEND_BYTE();
            return false;
        }
        // 2) Set USI to receive an acknowledge bit reply from master
        case STATE_RECEIVE_ACK_AFTER_SENDING_DATA: {
            // Next state -> STATE_CHECK_RECEIVED_ACK
            device_state = STATE_CHECK_RECEIVED_ACK;
            SET_USI_TO_RECEIVE_ACK();
            return false;
        }
        // Receive data mode:
        // ==================
        // 1) Set the USI to shift 8 bits in. When the 4-bit counter overflows,
        // it means that a byte has been received and this device should process it
        // on the next overflow state (STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK).
        case STATE_RECEIVE_DATA_BYTE: {
            // Next state -> STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK
            device_state = STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK;
            SET_USI_TO_RECEIVE_BYTE();
            return false;
        }
        // 2) Copy the received byte from USIDR to RX buffer and send ACK. After the
        // counter overflows, return to the previous state (STATE_RECEIVE_DATA_BYTE).
        // This mode's cycle should end when a stop condition is detected on the bus.
        case STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK: {
            // Put data into buffer
            rx_byte_count++;
            rx_head = ((rx_head + 1) & TWI_RX_BUFFER_MASK);
            rx_buffer[rx_head] = USIDR;
            // Next state -> STATE_RECEIVE_DATA_BYTE
            device_state = STATE_RECEIVE_DATA_BYTE;
            SET_USI_TO_SEND_ACK();
            return false;
        }        
    }
    // Clear the 4-bit counter overflow flag in USI status register after processing each
    // overflow state to allow detecting new interrupts that take this device to next states.
    USISR |= (1 << USI_OVERFLOW_FLAG);
    return false;
}

// ----------------------------------------------------------------------------
// USI TWI basic operations functions
// ----------------------------------------------------------------------------
// Set USI to detect start and shift 7 address bits + 1 direction bit in.
void SET_USI_TO_WAIT_FOR_TWI_ADDRESS(void) {
    SET_USI_TO_DETECT_TWI_START();  /* Detect start condition */
    SET_USI_TO_SHIFT_8_DATA_BITS(); /* Shift 8 bits */
}
// ............................................................................
// Set USI to send a byte.
inline void SET_USI_TO_SEND_BYTE(void) {
    SET_USI_SDA_AS_OUTPUT();        /* Drive the SDA line */
    SET_USI_TO_SHIFT_8_DATA_BITS(); /* Shift 8 bits */
}
// ............................................................................
// Set USI to receive a byte.
inline void SET_USI_TO_RECEIVE_BYTE(void) {
    SET_USI_SDA_AS_INPUT();         /* Float the SDA line */
    SET_USI_TO_SHIFT_8_DATA_BITS(); /* Shift 8 bits */
}
// ............................................................................
// Set USI to send an ACK bit.
inline void SET_USI_TO_SEND_ACK(void) {
    USIDR = 0;                      /* Clear the USI data register */
    SET_USI_SDA_AS_OUTPUT();        /* Drive the SDA line */
    SET_USI_TO_SHIFT_1_ACK_BIT();   /* Shift 1 bit */
}
// ............................................................................
// Set USI to receive an ACK bit.
inline void SET_USI_TO_RECEIVE_ACK(void) {
    USIDR = 0;                      /* Clear the USI data register */
    SET_USI_SDA_AS_INPUT();         /* Float the SDA line */
    SET_USI_TO_SHIFT_1_ACK_BIT();   /* Shift 1 bit */
}

// ----------------------------------------------------------------------------
// USI register configurations
// ----------------------------------------------------------------------------
// Configure USI control register to detect start condition.
inline void SET_USI_TO_DETECT_TWI_START(void) {
    USICR = (1 << TWI_START_COND_INT) | (0 << USI_OVERFLOW_INT) | /* Enable start condition interrupt, disable overflow interrupt */
            (1 << USIWM1) | (0 << USIWM0) | /* Set USI in Two-wire mode, don't hold SCL low when the 4-bit counter overflows */
            (1 << USICS1) | (0 << USICS0) | (0 << USICLK) | /* Clock Source = External (positive edge) for data register, External (both edges) for 4-Bit counter */
            (0 << USITC); /* No toggle clock-port pin (SCL) */              
}
// ............................................................................
// Configure USI control register to detect RESTART.
inline void SET_USI_TO_DETECT_TWI_RESTART(void) {
    USICR = (1 << TWI_START_COND_INT) | (1 << USI_OVERFLOW_INT) | /* Enable start condition interrupt, disable overflow interrupt */
            (1 << USIWM1) | (1 << USIWM0) | /* Set USI in Two-wire mode, hold SCL low when the 4-bit counter overflows */
            (1 << USICS1) | (0 << USICS0) | (0 << USICLK) | /* Clock Source = External (positive edge) for data register, External (both edges) for 4-Bit counter */
            (0 << USITC); /* No toggle clock-port pin (SCL) */    
}
// ............................................................................
// Clear all USI status register interrupt flags to prepare for new start conditions.
inline void SET_USI_TO_SHIFT_8_ADDRESS_BITS(void) {
    USISR = (1 << TWI_START_COND_FLAG) |
            (1 << USI_OVERFLOW_FLAG) |
            (1 << TWI_STOP_COND_FLAG) |
            (1 << TWI_COLLISION_FLAG) |
            (0x0 << USICNT0); /* Reset status register 4-bit counter to shift 8 bits (data byte to be received) */
}
// ............................................................................
// Clear all USI status register interrupt flags, except start condition.
void SET_USI_TO_SHIFT_8_DATA_BITS(void) {
    USISR = (0 << TWI_START_COND_FLAG) |
            (1 << USI_OVERFLOW_FLAG) |
            (1 << TWI_STOP_COND_FLAG) |
            (1 << TWI_COLLISION_FLAG) |
            (0x0 << USICNT0); /* Set status register 4-bit counter to shift 8 bits */    
}
// ............................................................................
// Clear all USI status register interrupt flags, except start condition.
inline void SET_USI_TO_SHIFT_1_ACK_BIT(void) {
    USISR = (0 << TWI_START_COND_FLAG) |
            (1 << USI_OVERFLOW_FLAG) |
            (1 << TWI_STOP_COND_FLAG) |
            (1 << TWI_COLLISION_FLAG) |
            (0x0E << USICNT0); /* Set status register 4-bit counter to shift 1 bit */
}

// ----------------------------------------------------------------------------
// GPIO TWI direction settings
// ----------------------------------------------------------------------------
// Drive the data line
inline void SET_USI_SDA_AS_OUTPUT(void) {
    DDR_USI |=  (1 << PORT_USI_SDA);
}
// ............................................................................
// Float the data line
inline void SET_USI_SDA_AS_INPUT(void) {
    DDR_USI &= ~(1 << PORT_USI_SDA);
}
// ............................................................................
// Drive the clock line
inline void SET_USI_SCL_AS_OUTPUT(void) {
    DDR_USI |=  (1 << PORT_USI_SCL);
}
// ............................................................................
// Float the clock line
inline void SET_USI_SCL_AS_INPUT(void) {
    DDR_USI &= ~(1 << PORT_USI_SCL);
}
// ............................................................................
// Drive the data and clock lines
inline void SET_USI_SDA_AND_SCL_AS_OUTPUT(void) {
    DDR_USI |= (1 << PORT_USI_SDA) | (1 << PORT_USI_SCL);
}
// ............................................................................
// Float the data and clock lines
inline void SET_USI_SDA_AND_SCL_AS_INPUT(void) {
    DDR_USI &= ~((1 << PORT_USI_SDA) | (1 << PORT_USI_SCL));
}
