/*           _                         _
 *       _  (_)                       | |
 *     _| |_ _ ____   ___  ____  _____| |
 *    (_   _) |    \ / _ \|  _ \| ___ | |
 *      | |_| | | | | |_| | | | | ____| |
 *       \__)_|_|_|_|\___/|_| |_|_____)\_)
 *
 *  Timonel - TWI Bootloader for TinyX4 MCUs
 *  Author: Gustavo Casanova
 *  ..............................................
 *  Version: 1.5 "Sandra" / 2020-09-19 "Canada"
 *  gustavo.casanova@nicebots.com
 */

// Includes
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
#define TWI_ADDR 11  // Timonel TWI default address: 11 (0x0B)
#endif               // 8 <= TWI_ADDR <= 35

// This bootloader ...
#define TIMONEL_VER_MJR 1  // Timonel version major number
#define TIMONEL_VER_MNR 5  // Timonel version major number

// Configuration checks
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

// Pointer-to-function type
typedef void (*const fptr_t)(void);

// Memory management and flags data pack
typedef struct m_pack {
    uint16_t page_addr;  // Flash memory page address
    uint8_t page_ix;     // Flash memory page index
    uint8_t flags;       // Bit: 8, 7, 6, 5: not used; 4: exit; 3: delete app; 2, 1: initialized
#if AUTO_PAGE_ADDR
    uint8_t app_reset_lsb;  // Application first byte: reset vector LSB
    uint8_t app_reset_msb;  // Application second byte: reset vector MSB
#endif                      // AUTO_PAGE_ADDR
} MemPack;                  // "Memory pack" structure

MemPack *p_mem_pack = NULL;

// Bootloader prototypes (without MemPack pointer)
inline static void ReceiveEvent(const uint8_t received_bytes) __attribute__((always_inline));
inline static void ResetPrescaler(void) __attribute__((always_inline));
inline static void RestorePrescaler(void) __attribute__((always_inline));
inline static void Reply_GETTMNLV(void) __attribute__((always_inline));
inline static void Reply_EXITTMNL(void) __attribute__((always_inline));
inline static void Reply_DELFLASH(void) __attribute__((always_inline));
#if (CMD_SETPGADDR || !(AUTO_PAGE_ADDR))
inline static void Reply_STPGADDR(const uint8_t *command) __attribute__((always_inline));
#endif  // CMD_SETPGADDR || !AUTO_PAGE_ADDR
inline static void Reply_WRITPAGE(const uint8_t *command) __attribute__((always_inline));
#if CMD_READFLASH
inline static void Reply_READFLSH(const uint8_t *command) __attribute__((always_inline));
#endif  // CMD_READFLASH
inline static void Reply_INITSOFT(void) __attribute__((always_inline));
#if CMD_READDEVS
inline static void Reply_READDEVS(void) __attribute__((always_inline));
#endif  // CMD_READDEVS
#if EEPROM_ACCESS
inline static void Reply_WRITEEPR(const uint8_t *command) __attribute__((always_inline));
inline static void Reply_READEEPR(const uint8_t *command) __attribute__((always_inline));
#endif  // EEPROM_ACCESS

// Main function
int main(void) {
    /* ___________________
      |                   | 
      |    Setup Block    |
      |___________________|
    */
    MCUSR = 0;  // Disable watchdog
#if defined(__AVR_ATtiny25__) | \
    defined(__AVR_ATtiny45__) | \
    defined(__AVR_ATtiny85__)
    WDTCR |= ((1 << WDCE) | (1 << WDE));
    WDTCR = ((1 << WDP2) | (1 << WDP1) | (1 << WDP0));  // 2 seconds timeout
#endif
#if defined(__AVR_ATtiny44__) | \
    defined(__AVR_ATtiny84__)
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = ((1 << WDP2) | (1 << WDP1) | (1 << WDP0));  // 2 seconds timeout
#endif
    cli();  // Disable interrupts
#if ENABLE_LED_UI
    LED_UI_DDR |= (1 << LED_UI_PIN);  // Set led pin data direction register for output
#endif                                // ENABLE_LED_UI
#if APP_AUTORUN
    uint8_t exit_delay = SHORT_EXIT_DLY;  // Exit-to-app delay when the bootloader isn't initialized
#endif                                    // APP_AUTORUN
    uint16_t led_delay = SHORT_LED_DLY;   // Blinking delay when the bootloader isn't initialized
#if AUTO_CLK_TWEAK                        // Automatic clock tweaking made at run time, based on low fuse value
//#pragma message "AUTO CLOCK TWEAKING SELECTED: Clock adjustments will be made at run time ..."
    uint8_t factory_osccal = OSCCAL;      // Preserve factory oscillator calibration
    if ((boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS) & 0x0F) == RCOSC_CLK_SRC) {
        // RC oscillator (8 MHz) clock source set in low fuse, calibrating oscillator up ...
        OSCCAL += OSC_FAST;  // Speed oscillator up for TWI to work
    } else if ((boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS) & 0x0F) == HFPLL_CLK_SRC) {
        // HF PLL (16 MHz) clock source set in low fuse. No clock tweaking needed ...
    } else {
        // Unknown clock source set in low fuse! the prescaler will be reset to 1 to use the external clock as is
        ResetPrescaler();  // If using an external CPU clock source, don't reduce its frequency
    }
    if (!((boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS) >> LFUSE_PRESC_BIT) & true)) {
        // Prescaler fuse bit set to divide clock by 8, setting the CPU prescaler division factor to 1
        ResetPrescaler();  // Reset prescaler to divide by 1
    }
#else  // Clock tweaking made at compile time, based on LOW_FUSE variable
#define XSTR(x) STR(x)
#define STR(x) #x
                                                         //#pragma message "CLOCK TWEAKING AT COMPILE TIME BASED ON LOW_FUSE VARIABLE: " XSTR(LOW_FUSE)
#if ((LOW_FUSE & 0x0F) == RCOSC_CLK_SRC)                                           // RC oscillator (8 MHz) clock source
//#pragma message "RC oscillator (8 MHz) clock source selected, calibrating oscillator up ..."
    uint8_t factory_osccal = OSCCAL;  // With 8 MHz clock source, preserve factory oscillator
    OSCCAL += OSC_FAST;               // calibration and speed it up for TWI to work.
#elif ((LOW_FUSE & 0x0F) == HFPLL_CLK_SRC)                                         // HF PLL (16 MHz) clock source
//#pragma message "HF PLL (16 MHz) clock source selected. No clock tweaking needed ..."
#else                                                                              // Unknown clock source
//#pragma GCC warning "UNKNOWN LOW_FUSE CLOCK SETTING! VALID VALUES ARE 0xE1, 0x61, 0xE2 and 0x62"
    ResetPrescaler();  // If using an external CPU clock source, don't reduce its frequency
#endif                                                                             // LOW_FUSE CLOCK SOURCE
#if ((LOW_FUSE & 0x80) == 0)                                                       // Prescaler dividing clock by 8
                                      //#pragma message "Prescaler dividing clock by 8, setting the CPU prescaler division factor to 1 ..."
    ResetPrescaler();                 // Reset prescaler to divide by 1
#endif                                                                             // LOW_FUSE PRESCALER BIT
#endif                                                                             // AUTO_CLK_TWEAK
    UsiTwiDriverInit();                                                            // Initialize the TWI driver
    p_receive_event = ReceiveEvent;                                                // Pointer to TWI receive event function
    __SPM_REG = (_BV(CTPB) | _BV(__SPM_ENABLE));                                   // Prepare to clear the temporary page buffer
    asm volatile("spm");                                                           // Run SPM instruction to complete the clearing
    static const fptr_t RunApplication = (const fptr_t)((TIMONEL_START - 2) / 2);  // Pointer to trampoline to app address
#if !(USE_WDT_RESET)
    static const fptr_t RestartTimonel = (const fptr_t)(TIMONEL_START / 2);  // Pointer to bootloader start address
#endif                                                                       // !USE_WDT_RESET
    bool slow_ops_enabled = false;                                           // Allow slow operations only after completing TWI handshake
    MemPack mem_pack;
    //MemPack *p_mem_pack = &mem_pack;  // Pointer to "memory pack" structure
    p_mem_pack = &mem_pack;  // Pointer to "memory pack" structure
    p_mem_pack->page_addr = 0x0000;
    p_mem_pack->page_ix = 0;
    p_mem_pack->flags = 0;
#if AUTO_PAGE_ADDR
    p_mem_pack->app_reset_lsb = 0x00;
    p_mem_pack->app_reset_msb = 0x00;
#endif  // AUTO_PAGE_ADDR
    /* ___________________
      |                   | 
      |     Main Loop     |
      |___________________|
    */
    for (;;) {
        /*......................................................
          . USI TWI INTERRUPT EMULATION [ START ]               .
          . Check the USI status register to verify whether      .
          . a TWI start condition handler should be triggered   .
          ......................................................
        */
        if (((USISR >> TWI_START_COND_FLAG) & true) && ((USICR >> TWI_START_COND_INT) & true)) {
            // If so, run the USI start handler ...
            TwiStartHandler();
        }
        /*......................................................
          . USI TWI INTERRUPT EMULATION [ OVERFLOW ]            .
          . Check the USI status register to verify whether a    .
          . 4-bit counter overflow handler should be triggered  .
          ......................................................
        */
        if (((USISR >> USI_OVERFLOW_FLAG) & true) && ((USICR >> USI_OVERFLOW_INT) & true)) {
            // If so, run the USI overflow handler ...
            //slow_ops_enabled = UsiOverflowHandler(p_mem_pack);
            slow_ops_enabled = UsiOverflowHandler();
        }
        /*..............................
          :                             .
          :   Bootloader initialized     .
          :   ----------------------    .
          ..............................
        */
#if !(TWO_STEP_INIT)
        if ((p_mem_pack->flags >> FL_INIT_1) & true) {
#else
        if (((p_mem_pack->flags >> FL_INIT_1) & true) && ((p_mem_pack->flags >> FL_INIT_2) & true)) {
#endif  // TWO_STEP_INIT
            /*....................
              :                   .
              :     Slow-Ops       .
              :     --------      .
              ....................
            */
            if (slow_ops_enabled == true) {
                slow_ops_enabled = false;
                // =========================================================
                // = Exit the bootloader & run the application (Slow-Op 1) =
                // =========================================================
                if ((p_mem_pack->flags >> FL_EXIT_TML) & true) {
#if CLEAR_BIT_7_R31
                    asm volatile("cbr r31, 0x80");  // Clear bit 7 of r31
#endif                                              // CLEAR_BIT_7_R31
#if AUTO_CLK_TWEAK
                    if ((boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS) & 0x0F) == RCOSC_CLK_SRC) {
                        OSCCAL = factory_osccal;  // Back the oscillator calibration to its original setting
                    }
                    if (!((boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS) >> LFUSE_PRESC_BIT) & true)) {
                        RestorePrescaler();  // Restore prescaler to divide by 8
                    }
#else
#if ((LOW_FUSE & 0x0F) == RCOSC_CLK_SRC)
                    OSCCAL = factory_osccal;  // Back the oscillator calibration to its original setting
#endif                                 // LOW_FUSE RC OSC
#if ((LOW_FUSE & 0x80) == 0)           // Prescaler dividing clock by 8
                    RestorePrescaler();       // Restore prescaler factor to divide by 8
#endif                                 // PRESCALER BIT
#endif                                 // AUTO_CLK_TWEAK
                    RunApplication();  // Exit to the application
                }
                // ==================================================
                // = Delete the application from memory (Slow-Op 2) =
                // ==================================================
                if ((p_mem_pack->flags >> FL_DEL_FLASH) & true) {
#if ENABLE_LED_UI
                    LED_UI_PORT |= (1 << LED_UI_PIN);  // Turn led on to indicate erasing ...
#endif                                                 // ENABLE_LED_UI
                    uint16_t page_to_del = TIMONEL_START;
                    while (page_to_del != RESET_PAGE) {
                        page_to_del -= SPM_PAGESIZE;
                        boot_page_erase(page_to_del);  // Erase flash memory ...
                    }
#if AUTO_CLK_TWEAK
                    if ((boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS) & 0x0F) == RCOSC_CLK_SRC) {
                        OSCCAL = factory_osccal;  // Back the oscillator calibration to its original setting
                    }
#else
#if ((LOW_FUSE & 0x0F) == RCOSC_CLK_SRC)
                    OSCCAL = factory_osccal;  // Back the oscillator calibration to its original setting
#endif  // LOW_FUSE RC OSC
#endif  // AUTO_CLK_TWEAK
#if !(USE_WDT_RESET)
                    RestartTimonel();  // Restart by jumping to Timonel start
#else
                    wdt_enable(WDTO_15MS);    // Restart by activating the watchdog timer
                    for (;;) {
                    };
#endif  // !USE_WDT_RESET
                }
                // ===========================================================================
                // = Write the received page to memory and prepare for a new one (Slow-Op 3) =
                // ===========================================================================
#if (APP_USE_TPL_PG || !(AUTO_PAGE_ADDR))
                if ((p_mem_pack->page_ix == SPM_PAGESIZE) && (p_mem_pack->page_addr < TIMONEL_START)) {
#else
                if ((p_mem_pack->page_ix == SPM_PAGESIZE) && (p_mem_pack->page_addr < TIMONEL_START - SPM_PAGESIZE)) {
#endif  // (APP_USE_TPL_PG || !(AUTO_PAGE_ADDR))
#if ENABLE_LED_UI
                    LED_UI_PORT ^= (1 << LED_UI_PIN);  // Turn led on and off to indicate writing ...
#endif                                                 // ENABLE_LED_UI
#if FORCE_ERASE_PG
                    boot_page_erase(p_mem_pack->page_addr);
#endif  // FORCE_ERASE_PG
                    boot_page_write(p_mem_pack->page_addr);
#if AUTO_PAGE_ADDR
                    if (p_mem_pack->page_addr == RESET_PAGE) {  // Calculate and write trampoline
                        uint16_t tpl = (((~((TIMONEL_START >> 1) - ((((p_mem_pack->app_reset_msb << 8) | p_mem_pack->app_reset_lsb) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
                        for (int i = 0; i < SPM_PAGESIZE - 2; i += 2) {
                            boot_page_fill((TIMONEL_START - SPM_PAGESIZE) + i, 0xFFFF);
                        }
                        boot_page_fill((TIMONEL_START - 2), tpl);
                        boot_page_write(TIMONEL_START - SPM_PAGESIZE);
                    }
#if APP_USE_TPL_PG
                    if (p_mem_pack->page_addr == (TIMONEL_START - SPM_PAGESIZE)) {
                        uint16_t tpl = (((~((TIMONEL_START >> 1) - ((((p_mem_pack->app_reset_msb << 8) | p_mem_pack->app_reset_lsb) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
                        // - Read the previous page to the bootloader start, write it to the temporary buffer.
                        const __flash uint8_t *mem_position;
                        for (uint8_t i = 0; i < SPM_PAGESIZE - 2; i += 2) {
                            mem_position = (void *)((TIMONEL_START - SPM_PAGESIZE) + i);
                            uint16_t page_data = (*mem_position & 0xFF);
                            page_data += ((*(++mem_position) & 0xFF) << 8);
                            boot_page_fill((TIMONEL_START - SPM_PAGESIZE) + i, page_data);
                        }
                        // - Check if the last two bytes of the trampoline page are not used by the application.
                        mem_position = (void *)(TIMONEL_START - 2);
                        uint16_t page_data = (*mem_position & 0xFF);
                        page_data += ((*(++mem_position) & 0xFF) << 8);
                        if (page_data != tpl) {
                            // If the application overwrites the trampoline bytes, delete it!
                            p_mem_pack->flags |= (1 << FL_DEL_FLASH);
                        }
                    }
#endif  // APP_USE_TPL_PG
                    p_mem_pack->page_addr += SPM_PAGESIZE;
#endif  // AUTO_PAGE_ADDR
                    p_mem_pack->page_ix = 0;
                }
            }
        /*..................................
          :                                 .
          :   Bootloader NOT initialized     .
          :   --------------------------    .
          :.................................
        */
        } else {
            if (led_delay-- == 0) {
#if ENABLE_LED_UI
                LED_UI_PORT ^= (1 << LED_UI_PIN);  // If Timonel isn't initialized, led blinks at LED_DLY intervals
#endif                                             // ENABLE_LED_UI
#if APP_AUTORUN
                if (exit_delay-- == 0) {
                    // ========================================
                    // = >>> Timeout: Run the application <<< =
                    // ========================================
#if AUTO_CLK_TWEAK
                    if ((boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS) & 0x0F) == RCOSC_CLK_SRC) {
                        OSCCAL = factory_osccal;  // Back the oscillator calibration to its original setting
                    }
                    if (!((boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS) >> LFUSE_PRESC_BIT) & true)) {
                        RestorePrescaler();  // Restore prescaler to divide by 8
                    }
#else
#if ((LOW_FUSE & 0x0F) == RCOSC_CLK_SRC)
                    OSCCAL = factory_osccal;  // Back the oscillator calibration to its original setting
#endif                                 // LOW_FUSE & 0x0F
#if ((LOW_FUSE & 0x80) == 0)           // Prescaler dividing clock by 8
                    RestorePrescaler();       // Restore prescaler factor to divide by 8
#endif                                 // LOW_FUSE & 0x80
#endif                                 // AUTO_CLK_TWEAK
                    RunApplication();  // Count from CYCLESTOEXIT to 0, then exit to the application
                }
#endif  // APP_AUTORUN
            }
        }
    }
    return 0;
}

/* __________________________
  |                          |
  |  TWI data receive event  |
  |__________________________|
*/
inline void ReceiveEvent(const uint8_t received_bytes) {
    static uint8_t command[MST_PACKET_SIZE * 2];
    for (uint8_t i = 0; i < received_bytes; i++) {
        command[i] = UsiTwiReceiveByte();  // Store the data sent by the TWI master in the data buffer
    }
    switch (command[0]) {
        case GETTMNLV: {
            Reply_GETTMNLV();
            return;
        }
        case EXITTMNL: {
            Reply_EXITTMNL();
            return;
        }
        case DELFLASH: {
            Reply_DELFLASH();
            return;
        }
#if (CMD_SETPGADDR || !(AUTO_PAGE_ADDR))
        case STPGADDR: {
            Reply_STPGADDR(command);
            return;
        }
#endif  // CMD_SETPGADDR || !AUTO_PAGE_ADDR
        case WRITPAGE: {
            Reply_WRITPAGE(command);
            return;
        }
#if CMD_READFLASH
        case READFLSH: {
            Reply_READFLSH(command);
            return;
        }
#endif  // CMD_READFLASH
#if TWO_STEP_INIT
        case INITSOFT: {
            Reply_INITSOFT();
            return;
        }
#endif  // TWO_STEP_INIT
#if CMD_READDEVS
        case READDEVS: {
            Reply_READDEVS();
            return;
        }
#endif  // CMD_READDEVS
#if EEPROM_ACCESS
        case WRITEEPR: {
            Reply_WRITEEPR(command);
            return;
        }
        case READEEPR: {
            Reply_READEEPR(command);
            return;
        }
#endif  // EEPROM_ACCESS
        default: {
            UsiTwiTransmitByte(UNKNOWNC);
        }
    }
}

/* ____________________
  |                    |
  |   Reply_GETTMNLV   |
  |____________________|
*/
inline void Reply_GETTMNLV(void) {
    const __flash uint8_t *mem_position;
    mem_position = (void *)(TIMONEL_START - 1);
    uint8_t reply[GETTMNLV_RPLYLN];
    reply[0] = ACKTMNLV;
    reply[1] = ID_CHAR_3;                                    // "T" Signature
    reply[2] = TIMONEL_VER_MJR;                              // Major version number
    reply[3] = TIMONEL_VER_MNR;                              // Minor version number
    reply[4] = TML_FEATURES;                                 // Optional features
    reply[5] = TML_EXT_FEATURES;                             // Extended optional features
    reply[6] = ((TIMONEL_START & 0xFF00) >> 8);              // Bootloader start address MSB
    reply[7] = (TIMONEL_START & 0xFF);                       // Bootloader start address LSB
    reply[8] = *mem_position;                                // Trampoline second byte (MSB)
    reply[9] = *(--mem_position);                            // Trampoline first byte (LSB)
    reply[10] = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);  // Low fuse bits
    reply[11] = OSCCAL;                                      // Internal RC oscillator calibration
    p_mem_pack->flags |= (1 << FL_INIT_1);                   // First-step of single or two-step initialization
#if ENABLE_LED_UI
    LED_UI_PORT &= ~(1 << LED_UI_PIN);  // Turn led off to indicate initialization
#endif                                  // ENABLE_LED_UI
    for (uint8_t i = 0; i < GETTMNLV_RPLYLN; i++) {
        UsiTwiTransmitByte(reply[i]);
    }
}

/* ____________________
  |                    |
  |   Reply_EXITTMNL   |
  |____________________|
*/
inline void Reply_EXITTMNL(void) {
    UsiTwiTransmitByte(ACKEXITT);
    p_mem_pack->flags |= (1 << FL_EXIT_TML);
    p_mem_pack->flags |= (1 << FL_INIT_1);
}

/* ____________________
  |                    |
  |   Reply_DELFLASH   |
  |____________________|
*/
inline void Reply_DELFLASH(void) {
    UsiTwiTransmitByte(ACKDELFL);
    p_mem_pack->flags |= (1 << FL_DEL_FLASH);
}

#if (CMD_SETPGADDR || !(AUTO_PAGE_ADDR))
/* ____________________
  |                    |
  |   Reply_STPGADDR   |
  |____________________|
*/
inline void Reply_STPGADDR(const uint8_t *command) {
    uint8_t reply[STPGADDR_RPLYLN] = {0};
    p_mem_pack->page_addr = ((command[1] << 8) + command[2]);  // Sets the flash memory page base address
    p_mem_pack->page_addr &= ~(SPM_PAGESIZE - 1);              // Keep only pages' base addresses
    reply[0] = AKPGADDR;
    reply[1] = (uint8_t)(command[1] + command[2]);  // Returns the sum of MSB and LSB of the page address
    for (uint8_t i = 0; i < STPGADDR_RPLYLN; i++) {
        UsiTwiTransmitByte(reply[i]);
    }
}
#endif  // (CMD_SETPGADDR || !AUTO_PAGE_ADDR)

/* ____________________
  |                    |
  |   Reply_WRITPAGE   |
  |____________________|
*/
inline void Reply_WRITPAGE(const uint8_t *command) {
    uint8_t reply[WRITPAGE_RPLYLN] = {0};
    reply[0] = ACKWTPAG;
    if ((p_mem_pack->page_addr + p_mem_pack->page_ix) == RESET_PAGE) {
#if AUTO_PAGE_ADDR
        p_mem_pack->app_reset_lsb = command[1];
        p_mem_pack->app_reset_msb = command[2];
#endif  // AUTO_PAGE_ADDR
    // This section modifies the reset vector to point to this bootloader.
    // WARNING: This only works when CMD_SETPGADDR is disabled. If CMD_SETPGADDR is enabled,
    // the reset vector modification MUST BE done by the TWI master's upload program.
    // Otherwise, Timonel won't have the execution control after power-on reset.
        boot_page_fill((RESET_PAGE), (0xC000 + ((TIMONEL_START / 2) - 1)));
        reply[1] += (uint8_t)((command[2]) + command[1]);  // Reply checksum accumulator
        p_mem_pack->page_ix += 2;
        for (uint8_t i = 3; i < (MST_PACKET_SIZE + 1); i += 2) {
            boot_page_fill((p_mem_pack->page_addr + p_mem_pack->page_ix), ((command[i + 1] << 8) | command[i]));
            reply[1] += (uint8_t)((command[i + 1]) + command[i]);
            p_mem_pack->page_ix += 2;
        }
    } else {
        for (uint8_t i = 1; i < (MST_PACKET_SIZE + 1); i += 2) {
            boot_page_fill((p_mem_pack->page_addr + p_mem_pack->page_ix), ((command[i + 1] << 8) | command[i]));
            reply[1] += (uint8_t)((command[i + 1]) + command[i]);
            p_mem_pack->page_ix += 2;
        }
    }
#if CHECK_PAGE_IX
    if ((reply[1] != command[MST_PACKET_SIZE + 1]) || (p_mem_pack->page_ix > SPM_PAGESIZE)) {
#else
    if (reply[1] != command[MST_PACKET_SIZE + 1]) {
#endif                                             // CHECK_PAGE_IX
        p_mem_pack->flags |= (1 << FL_DEL_FLASH);  // If checksums don't match, safety payload deletion ...
        reply[1] = 0;
    }
    for (uint8_t i = 0; i < WRITPAGE_RPLYLN; i++) {
        UsiTwiTransmitByte(reply[i]);
    }
}

#if CMD_READFLASH
/* ____________________
  |                    |
  |   Reply_READFLSH   |
  |____________________|
*/
inline void Reply_READFLSH(const uint8_t *command) {
    const uint8_t reply_len = (command[3] + 2);  // Reply length: ack + memory positions requested + checksum
    uint8_t reply[reply_len];
    reply[0] = ACKRDFSH;
    reply[reply_len - 1] = 0;  // Checksum initialization
    // Point the initial memory position to the received address, then
    // advance to fill the reply with the requested data amount.
    const __flash uint8_t *mem_position;
    mem_position = (void *)((command[1] << 8) + command[2]);
    for (uint8_t i = 1; i < command[3] + 1; i++) {
        reply[i] = (*(mem_position++) & 0xFF);        // Actual memory position data
        reply[reply_len - 1] += (uint8_t)(reply[i]);  // Checksum accumulator
    }
    reply[reply_len - 1] += (uint8_t)(command[1]);  // Add Received address MSB to checksum
    reply[reply_len - 1] += (uint8_t)(command[2]);  // Add Received address MSB to checksum
    for (uint8_t i = 0; i < reply_len; i++) {
        UsiTwiTransmitByte(reply[i]);
    }
#if ENABLE_LED_UI
    LED_UI_PORT ^= (1 << LED_UI_PIN);  // Blinks whenever a memory data block is sent
#endif                                 // ENABLE_LED_UI
}
#endif  // CMD_READFLASH

/* ____________________
  |                    |
  |   Reply_INITSOFT   |
  |____________________|
*/
inline void Reply_INITSOFT(void) {
    p_mem_pack->flags |= (1 << FL_INIT_2);  // Two-step init step 1: receive INITSOFT command
    UsiTwiTransmitByte(ACKINITS);
}

#if CMD_READDEVS
/* ____________________
  |                    |
  |   Reply_READDEVS   |
  |____________________|
*/
inline void Reply_READDEVS(void) {
    uint8_t reply[READDEVS_RPLYLN];
    reply[0] = ACKRDEVS;
    reply[1] = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);       // Low fuse bits
    reply[2] = boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS);      // High fuse bits
    reply[3] = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS);  // Extended fuse bits
    reply[4] = boot_lock_fuse_bits_get(GET_LOCK_BITS);           // Device lock bits
    reply[5] = boot_signature_byte_get(0x00);                    // Signature byte 0
    reply[6] = boot_signature_byte_get(0x02);                    // Signature byte 1
    reply[7] = boot_signature_byte_get(0x04);                    // Signature byte 2
    reply[8] = boot_signature_byte_get(0x01);                    // Calibration data for internal oscillator at 8.0 MHz
    reply[9] = boot_signature_byte_get(0x03);                    // Calibration data for internal oscillator at 6.4 MHz
    for (uint8_t i = 0; i < READDEVS_RPLYLN; i++) {
        UsiTwiTransmitByte(reply[i]);
    }
}
#endif  // READDEVS

#if EEPROM_ACCESS
/* ____________________
  |                    |
  |   Reply_WRITEEPR   |
  |____________________|
*/
inline void Reply_WRITEEPR(const uint8_t *command) {
    uint8_t reply[WRITEEPR_RPLYLN] = {0};
    uint16_t eeprom_addr = ((command[1] << 8) + command[2]);  // Set the EEPROM address
    eeprom_addr &= E2END;                                     // Keep only valid EEPROM addresses
    reply[0] = ACKWTEEP;
    reply[1] = (uint8_t)(command[1] + command[2] + command[3]);  // Returns the sum of EEPROM address MSB, LSB and data byte
    eeprom_update_byte((uint8_t *)eeprom_addr, command[3]);
    for (uint8_t i = 0; i < WRITEEPR_RPLYLN; i++) {
        UsiTwiTransmitByte(reply[i]);
    }
}

/* ____________________
  |                    |
  |   Reply_READEEPR   |
  |____________________|
*/
inline void Reply_READEEPR(const uint8_t *command) {
    uint8_t reply[READEEPR_RPLYLN] = {0};
    uint16_t eeprom_addr = ((command[1] << 8) + command[2]);  // Set the EEPROM address
    eeprom_addr &= E2END;                                     // Keep only valid EEPROM addresses
    reply[0] = ACKRDEEP;
    reply[1] = (uint8_t)eeprom_read_byte((uint8_t *)eeprom_addr);
    reply[2] = (uint8_t)(command[1] + command[2] + reply[1]);  // Returns the sum of EEPROM address MSB, LSB and data byte
    for (uint8_t i = 0; i < READEEPR_RPLYLN; i++) {
        UsiTwiTransmitByte(reply[i]);
    }
}
#endif  // EEPROM_ACCESS

/* ____________________
  |                    |
  |   ResetPrescaler   |
  |____________________|
*/
inline void ResetPrescaler(void) {
    // Set the CPU prescaler division factor to 1
    CLKPR = (1 << CLKPCE);  // Prescaler enable
    CLKPR = 0x00;           // Clock division factor 1 (0000)
}

/* ______________________
  |                      |
  |   RestorePrescaler   |
  |______________________|
*/
inline void RestorePrescaler(void) {
    // Set the CPU prescaler division factor to 8
    CLKPR = (1 << CLKPCE);                    // Prescaler enable
    CLKPR = ((1 << CLKPS1) | (1 << CLKPS0));  // Clock division factor 8 (0011)
}
