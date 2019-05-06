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
 *  Version: 1.3 "Sandra" / 2019-04-28 (GOOD-SUB)
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
    #define TWI_ADDR    8                   /* Timonel TWI default address: 08 (0x08) */
#endif /* 8 <= TWI_ADDR <= 35 */

/* This bootloader ... */
#define TIMONEL_VER_MJR 1                   /* Timonel version major number   */
#define TIMONEL_VER_MNR 3                   /* Timonel version major number   */

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
                                
#if ((MST_PACKET_SIZE > (TWI_RX_BUFFER_SIZE / 2)) || ((SLV_PACKET_SIZE > (TWI_TX_BUFFER_SIZE / 2))))
    #pragma GCC warning "Don't set transmission data size too high to avoid affecting the TWI reliability!"
#endif

#if ((CYCLESTOEXIT > 0) && (CYCLESTOEXIT < 10))
    #pragma GCC warning "Do not set CYCLESTOEXIT too low, it could make difficult for TWI master to initialize on time!"
#endif

// Type definitions
typedef void (* const fptr_t)(void);              /* Pointer-to-function type */
typedef enum {                                    /* TWI driver operational modes */
    STATE_CHECK_ADDRESS,
    STATE_SEND_DATA,
    STATE_WAIT_ACK_AFTER_SEND_DATA,
    STATE_CHECK_ACK_AFTER_SEND_DATA,
    STATE_WAIT_DATA_RECEPTION,
    STATE_RECEIVE_DATA_AND_SEND_ACK   
} OperationalState;

// Bootloader globals
uint8_t command[(MST_PACKET_SIZE * 2) + 2] = { 0 }; /* Command received from TWI master */
uint8_t flags = 0;                                /* Bit: 8,7,6: not used; 5: slow-ops; 4: exit; 3: delete flash; 2, 1: initialized */
uint8_t page_ix = 0;                              /* Flash memory page index */
uint16_t page_addr = 0x0000;                      /* Flash memory page address */
#if AUTO_TPL_CALC
uint8_t reset_lsb = 0xFF;                         /* Application first byte: reset vector LSB */
uint8_t reset_msb = 0xFF;                         /* Application second byte: reset vector MSB */
#endif /* AUTO_TPL_CALC */
static const fptr_t RunApplication = (const fptr_t)((TIMONEL_START - 2) / 2); /* Pointer to to-application trampoline address */
#if !(USE_WDT_RESET)
static const fptr_t RestartTimonel = (const fptr_t)(TIMONEL_START / 2); /* Pointer to bootloader restart address */
#endif /* !USE_WDT_RESET */

// USI TWI driver globals
uint8_t rx_buffer[TWI_RX_BUFFER_SIZE];
uint8_t tx_buffer[TWI_TX_BUFFER_SIZE];
uint8_t rx_byte_count;                            /* Received byte quantity in RX buffer */
uint8_t tx_byte_count;                            /* Byte quantity to transmit in TX buffer */ 
uint8_t rx_head;
uint8_t rx_tail;
uint8_t tx_head;
uint8_t tx_tail;
OperationalState device_state;

// Bootloader prototypes
inline void ReceiveEvent(uint8_t) __attribute__((always_inline));
inline void RequestEvent(void) __attribute__((always_inline));

// USI TWI driver prototypes
void UsiTwiTransmitByte(uint8_t);
uint8_t UsiTwiReceiveByte(void);
inline void UsiTwiDriverInit(void) __attribute__((always_inline));
inline void TwiStartHandler(void) __attribute__((always_inline));
inline void UsiOverflowHandler(void) __attribute__((always_inline));

// USI TWI driver basic operations prototypes
inline void SET_USI_TO_WAIT_FOR_TWI_ADDRESS(void) __attribute__((always_inline));
inline void SET_USI_TO_SEND_DATA(void) __attribute__((always_inline));
inline void SET_USI_TO_RECEIVE_DATA(void) __attribute__((always_inline));
inline void SET_USI_TO_SEND_ACK(void) __attribute__((always_inline));
inline void SET_USI_TO_WAIT_ACK(void) __attribute__((always_inline));
inline void SET_USI_TO_DETECT_TWI_START(void) __attribute__((always_inline));
inline void SET_USI_TO_DETECT_TWI_RESTART(void) __attribute__((always_inline));
inline void SET_USI_TO_SHIFT_8_ADDRESS_BITS(void) __attribute__((always_inline));
inline void SET_USI_TO_SHIFT_8_DATA_BITS(void) __attribute__((always_inline));
inline void SET_USI_TO_SHIFT_1_ACK_BIT(void) __attribute__((always_inline));

// USI TWI driver direction setting prototypes
inline void SET_USI_SDA_AS_OUTPUT(void) __attribute__((always_inline));
inline void SET_USI_SDA_AS_INPUT(void) __attribute__((always_inline));
inline void SET_USI_SCL_AS_OUTPUT(void) __attribute__((always_inline));
inline void SET_USI_SCL_AS_INPUT(void) __attribute__((always_inline));
inline void SET_USI_SDA_AND_SCL_AS_OUTPUT(void) __attribute__((always_inline));
inline void SET_USI_SDA_AND_SCL_AS_INPUT(void) __attribute__((always_inline));

volatile uint8_t kelonio = 0;

// Main function
int main(void) {
    /*  ___________________
       |                   | 
       |    Setup Block    |
       |___________________|
    */
    MCUSR = 0;                                    /* Disable watchdog */
    WDTCR = ((1 << WDCE) | (1 << WDE));
    WDTCR = ((1 << WDP2) | (1 << WDP1) | (1 << WDP0));
    cli();                                        /* Disable Interrupts */
#if ENABLE_LED_UI
    LED_UI_DDR |= (1 << LED_UI_PIN);              /* Set led pin data direction register for output */
#endif /* ENABLE_LED_UI */

kelonio = OSCCAL;

#if !(MODE_16_MHZ)
    OSCCAL += (OSC_OFFSET - kelonio);                         /* With clock settings below 16MHz, speed up for better TWI performance */
#endif /* 16_MHZ_MODE */
#if SET_PRESCALER
    CLKPR = (1 << CLKPCE);                        /* Set the CPU prescaler division factor = 1 */
    CLKPR = 0x00;
#endif /* SET_PRESCALER */
    UsiTwiDriverInit();                           /* Initialize the TWI driver */
    __SPM_REG = (_BV(CTPB) | \
                 _BV(__SPM_ENABLE));              /* Clear the temporary page buffer */
    asm volatile("spm");
    uint8_t exit_delay = CYCLESTOEXIT;            /* Delay to exit bootloader and run the application if not initialized */
    uint16_t led_delay = CYCLESTOBLINK;           /* Delay for led blink */

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
            TwiStartHandler();                    /* If so, run the USI start handler ... */
        }       
        /* ......................................................
           . TWI Interrupt Emulation .......................... .
           . Check the USI status register to verify whether a  .
           . 4-bit counter overflow handler should be triggered .
           ......................................................
        */
        if (((USISR >> USI_OVERFLOW_FLAG) & true) && ((USICR >> USI_OVERFLOW_INT) & true)) {
            UsiOverflowHandler();                 /* If so, run the USI overflow handler ... */
        }
#if !(TWO_STEP_INIT)
        if ((flags >> FL_INIT_1) & true) {
#endif /* !TWO_STEP_INIT */
#if TWO_STEP_INIT
        if (((flags >> FL_INIT_1) & true) && ((flags >> FL_INIT_2) & true)) {
#endif /* TWO_STEP_INIT */
            // ======================================
            // =   \\\ Bootloader initialized ///   =
            // ======================================           
            if ((flags >> FL_SLOW_OPS) & true) {
                flags &= ~(1 << FL_SLOW_OPS);           
                // =======================================================
                // = Exit the bootloader & run the application (Slow Op) =
                // =======================================================
                if ((flags >> FL_EXIT_TML) & true) {
                    asm volatile("cbr r31, 0x80");          /* Clear bit 7 of r31 */
#if !(MODE_16_MHZ)
                    OSCCAL -= (OSC_OFFSET - kelonio);                   /* Back the oscillator calibration to its original setting */
#endif /* 16_MHZ_MODE */
                    RunApplication();                       /* Exit to the application */
                }
                // ================================================
                // = Delete the application from memory (Slow Op) =
                // ================================================
                if ((flags >> FL_DEL_FLASH) & true) {
#if ENABLE_LED_UI                   
                    LED_UI_PORT |= (1 << LED_UI_PIN);       /* Turn led on to indicate erasing ... */
#endif /* ENABLE_LED_UI */
                    uint16_t pageAddress = TIMONEL_START;   /* Erase flash ... */
                    while (pageAddress != RESET_PAGE) {
                        pageAddress -= PAGE_SIZE;
                        boot_page_erase(pageAddress);
                    }
#if !(MODE_16_MHZ)
                    OSCCAL -= (OSC_OFFSET - kelonio);                   /* Back the oscillator calibration to its original setting */
#endif /* 16_MHZ_MODE */
#if !(USE_WDT_RESET)
                    RestartTimonel();                       /* Restart the bootloader by jumping to Timonel start */
#else
                    wdt_enable(WDTO_15MS);                  /* Restart the bootloader by activating the watchdog timer */
                    for (;;) {};
#endif /* !USE_WDT_RESET */                    
                }
#if (APP_USE_TPL_PG || !(AUTO_TPL_CALC))
                // =========================================================================
                // = Write the received page to memory and prepare for a new one (Slow Op) =
                // =========================================================================    
                if ((page_ix == PAGE_SIZE) && (page_addr < TIMONEL_START)) {
#else
                if ((page_ix == PAGE_SIZE) && (page_addr < TIMONEL_START - PAGE_SIZE)) {
#endif /* APP_USE_TPL_PG || !(AUTO_TPL_CALC) */
#if ENABLE_LED_UI
                    LED_UI_PORT ^= (1 << LED_UI_PIN);       /* Turn led on and off to indicate writing ... */
#endif /* ENABLE_LED_UI */
#if FORCE_ERASE_PG
                    boot_page_erase(page_addr);
#endif /* FORCE_ERASE_PG */                    
                    boot_page_write(page_addr);
#if AUTO_TPL_CALC
                    if (page_addr == RESET_PAGE) {      /* Calculate and write trampoline */
                        uint16_t tpl = (((~((TIMONEL_START >> 1) - ((((reset_msb << 8) | reset_lsb) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
                        for (int i = 0; i < PAGE_SIZE - 2; i += 2) {
                            boot_page_fill((TIMONEL_START - PAGE_SIZE) + i, 0xFFFF);
                        }
                        boot_page_fill((TIMONEL_START - 2), tpl);
                        boot_page_write(TIMONEL_START - PAGE_SIZE);                        
                    }
#if APP_USE_TPL_PG
                    if (page_addr == (TIMONEL_START - PAGE_SIZE)) {
                        uint16_t tpl = (((~((TIMONEL_START >> 1) - ((((reset_msb << 8) | reset_lsb) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
                        // - Read the previous page to the bootloader start, write it to the temporary buffer.
                        const __flash unsigned char * flashAddr;
                        for (uint8_t i = 0; i < PAGE_SIZE - 2; i += 2) {
                            flashAddr = (void *)((TIMONEL_START - PAGE_SIZE) + i);
                            uint16_t pgData = (*flashAddr & 0xFF);
                            pgData += ((*(++flashAddr) & 0xFF) << 8); 
                            boot_page_fill((TIMONEL_START - PAGE_SIZE) + i, pgData);
                        }
                        // - Check if the last two bytes of the trampoline page are 0xFF.
                        flashAddr = (void *)(TIMONEL_START - 2);
                        uint16_t pgData = (*flashAddr & 0xFF);
                        pgData += ((*(++flashAddr) & 0xFF) << 8);
                        if (pgData == 0xFFFF) {
                            // -- If yes, then the application fits in memory, flash the trampoline bytes again.                            
                            boot_page_fill(TIMONEL_START - 2, tpl);
                            boot_page_erase(TIMONEL_START - PAGE_SIZE);
                            boot_page_write(TIMONEL_START - PAGE_SIZE);
                        }
                        else {
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
        else {
            // ======================================
            // = \\\ Bootloader not initialized /// =
            // ======================================
            if (led_delay-- == 0) {
#if ENABLE_LED_UI               
                LED_UI_PORT ^= (1 << LED_UI_PIN);           /* Blinks on each main loop pass at CYCLESTOWAIT intervals */
#endif /* ENABLE_LED_UI */                
                if (exit_delay-- == 0) {
                    // ==========================================
                    // = >>> Run the application by timeout <<< =
                    // ==========================================
#if !(MODE_16_MHZ)
                    OSCCAL -= (OSC_OFFSET - kelonio);                   /* Back the oscillator calibration to its original setting */
#endif /* 16_MHZ_MODE */
                    RunApplication();                       /* Count from CYCLESTOEXIT to 0, then exit to the application */
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
inline void ReceiveEvent(uint8_t commandBytes) {
    for (uint8_t i = 0; i < commandBytes; i++) {
        command[i] = UsiTwiReceiveByte();                   /* Store the data sent by the TWI master in the data buffer */
    }
}

/*  ________________________
   |                        |
   | TWI data request event |
   |________________________|
*/
inline void RequestEvent(void) {
    switch (command[0]) {
        // ******************
        // * GETTMNLV Reply *
        // ******************
        case GETTMNLV: {
#if CHECK_EMPTY_FL
            #define GETTMNLV_RPLYLN 11
#else
            #define GETTMNLV_RPLYLN 10
#endif /* CHECK_EMPTY_FL */
            const __flash unsigned char * flashAddr;
            flashAddr = (void *)(TIMONEL_START - 1); 
            uint8_t reply[GETTMNLV_RPLYLN] = { 0 };
            reply[0] = ACKTMNLV;
            reply[1] = ID_CHAR_3;                                   /* "T" Signature */            
            reply[2] = TIMONEL_VER_MJR;                             /* Major version number */
            reply[3] = TIMONEL_VER_MNR;                             /* Minor version number */
            reply[4] = TML_FEATURES;                                /* Optional features */
            reply[5] = ((TIMONEL_START & 0xFF00) >> 8);             /* Start address MSB */
            reply[6] = (TIMONEL_START & 0xFF);                      /* Start address LSB */
            reply[7] = (*flashAddr & 0xFF);                         /* Trampoline second byte (MSB) */
            reply[8] = (*(--flashAddr) & 0xFF);                     /* Trampoline first byte (LSB) */
            //reply[9] = LOW_FUSE;                                    /* AVR low fuse value */
            reply[9] = OSCCAL;                                    /* Kelonio */
#if CHECK_EMPTY_FL
            for (uint16_t mPos = 0; mPos < 100; mPos++) {           /* Check the first 100 memory positions to determine if  */
                flashAddr = (void *)(mPos);                         /* there is an application (or some other data) loaded.  */
                reply[10] += (uint8_t)~(*flashAddr);                    
            }
#endif /* CHECK_EMPTY_FL */
#if !(TWO_STEP_INIT)
            flags |= (1 << FL_INIT_1);                              /* Single-step init */
#endif /* !TWO_STEP_INIT */
#if TWO_STEP_INIT
            flags |= (1 << FL_INIT_2);                              /* Two-step init step 2: receive GETTMNLV command */
#endif /* TWO_STEP_INIT */
#if ENABLE_LED_UI
            LED_UI_PORT &= ~(1 << LED_UI_PIN);                      /* Turn led off to indicate initialization */
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
#if (CMD_STPGADDR || !(AUTO_TPL_CALC))
        // ******************
        // * STPGADDR Reply *
        // ******************
        case STPGADDR: {
            #define STPGADDR_RPLYLN 2
            uint8_t reply[STPGADDR_RPLYLN] = { 0 };
            page_addr = ((command[1] << 8) + command[2]);           /* Sets the flash memory page base address */
            page_addr &= ~(PAGE_SIZE - 1);                          /* Keep only pages' base addresses */
            reply[0] = AKPGADDR;
            reply[1] = (uint8_t)(command[1] + command[2]);          /* Returns the sum of MSB and LSB of the page address */
            for (uint8_t i = 0; i < STPGADDR_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            return;
        }
#endif /* CMD_STPGADDR || !AUTO_TPL_CALC */
        // ******************
        // * WRITPAGE Reply *
        // ******************
        case WRITPAGE: {
            #define WRITPAGE_RPLYLN 2
            uint8_t reply[WRITPAGE_RPLYLN] = { 0 };
            reply[0] = ACKWTPAG;
            if ((page_addr + page_ix) == RESET_PAGE) {
#if AUTO_TPL_CALC
                reset_lsb = command[1];
                reset_msb = command[2];
#endif /* AUTO_TPL_CALC */
                // This section modifies the reset vector to point to this bootloader.
                // WARNING: This only works when CMD_STPGADDR is disabled. If CMD_STPGADDR is enabled,
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
            }
            else {
                for (uint8_t i = 1; i < (MST_PACKET_SIZE + 1); i += 2) {
                    boot_page_fill((page_addr + page_ix), ((command[i + 1] << 8) | command[i]));
                    reply[1] += (uint8_t)((command[i + 1]) + command[i]);
                    page_ix += 2;
                }
            }
            if ((reply[1] != command[MST_PACKET_SIZE + 1]) || (page_ix > PAGE_SIZE)) {
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
            const uint8_t ackLng = (command[3] + 2);                /* Fourth byte received determines the size of reply data */
            uint8_t reply[ackLng];
            if ((command[3] >= 1) & (command[3] <= (SLV_PACKET_SIZE + 2) * 2) & ((uint8_t)(command[0] + command[1] + command[2] + command[3]) == command[4])) {
                reply[0] = ACKRDFSH;
                page_addr = ((command[1] << 8) + command[2]);       /* Sets the flash memory page base address */
                reply[ackLng - 1] = 0;                              /* Checksum initialization */
                const __flash unsigned char * flashAddr;
                flashAddr = (void *)page_addr; 
                for (uint8_t i = 1; i < command[3] + 1; i++) {
                    reply[i] = (*(flashAddr++) & 0xFF);             /* Actual flash data */
                    reply[ackLng - 1] += (uint8_t)(reply[i]);       /* Checksum accumulator to be sent in the last byte of the reply */
                }                
                for (uint8_t i = 0; i < ackLng; i++) {
                    UsiTwiTransmitByte(reply[i]);                   
                }
            }
            else {
                UsiTwiTransmitByte(UNKNOWNC);                       /* Incorrect operand value received */
            }
#if ENABLE_LED_UI               
            LED_UI_PORT ^= (1 << LED_UI_PIN);                       /* Blinks on each memory data block sent */
#endif /* ENABLE_LED_UI */          
            return;
        }
#endif /* CMD_READFLASH */
#if TWO_STEP_INIT
        // ******************
        // * INITSOFT Reply *
        // ******************
        case INITSOFT: {
            flags |= (1 << FL_INIT_1);                              /* Two-step init step 1: receive INITSOFT command */
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

/////////////////////////////////////////////////////////////////////////////
////////////       ALL USI TWI DRIVER CODE BELOW THIS LINE       ////////////
/////////////////////////////////////////////////////////////////////////////

/*  ___________________________
   |                           |
   | USI TWI byte transmission |
   |___________________________|
*/
void UsiTwiTransmitByte(uint8_t data_byte) {
    while (tx_byte_count++ == TWI_TX_BUFFER_SIZE) {
        // Wait until there is free space in the TX buffer.
    };
    tx_buffer[tx_head] = data_byte; /* Write the data byte into the TX buffer */
    tx_head = (tx_head + 1) & TWI_TX_BUFFER_MASK;
}

/*  ___________________________
   |                           |
   | USI TWI byte reception    |
   |___________________________|
*/
inline uint8_t UsiTwiReceiveByte(void) {
    uint8_t received_byte;
    while (!rx_byte_count--) {
        // Wait until a byte is received into the RX buffer.
    };
    received_byte = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) & TWI_RX_BUFFER_MASK; /* Calculate the buffer index */
    return received_byte; /* Return the received byte from the buffer */
}

/*  _______________________________
   |                               |
   | USI TWI driver initialization |
   |_______________________________|
*/
void UsiTwiDriverInit(void) {
    // In Two Wire mode (USIWM1, USIWM0 = 1X), the slave USI will pull SCL
    // low when a start condition is detected or a counter overflow (only
    // for USIWM1, USIWM0 = 11).  This inserts a wait state.  SCL is released
    // by the ISRs (USI_START_vect and USI_OVERFLOW_vect).
    rx_tail = rx_head = rx_byte_count = 0; /* Flush TWI RX buffers */
    tx_tail = tx_head = tx_byte_count = 0; /* Flush TWI TX buffers */
    SET_USI_SDA_AND_SCL_AS_OUTPUT(); /* Set SCL and SDA as output */
    PORT_USI |= (1 << PORT_USI_SDA); /* Set SDA high */
    PORT_USI |= (1 << PORT_USI_SCL); /* Set SCL high */
    SET_USI_SDA_AS_INPUT();          /* Set SDA as input */
    SET_USI_TO_WAIT_FOR_TWI_ADDRESS(); /* Wait for TWI start condition and address from master */
}

/*  _______________________________________________________
   |                                                       |
   | TWI start condition handler (Interrupt-like function) |
   |_______________________________________________________|
*/
inline void TwiStartHandler(void) {
    device_state = STATE_CHECK_ADDRESS; /* Set default starting conditions for a new TWI package */
    SET_USI_SDA_AS_INPUT(); /* Float the SDA line */
    while ((PIN_USI & (1 << PORT_USI_SCL)) && (!(PIN_USI & (1 << PORT_USI_SDA)))) {
        // Wait for SCL to go low to ensure the start condition has completed.
        // The start detector will hold SCL low.
    }
    // If a stop condition arises then leave this function to prevent waiting forever.
    // Don't use USISR to test for stop condition as in application note AVR312
    // because the stop condition flag is going to be set from the last TWI sequence.
    if (!(PIN_USI & (1 << PIN_USI_SDA))) {  /*** Stop condition NOT DETECTED ***/
        SET_USI_TO_DETECT_TWI_RESTART();
    }
    else {                                  /*** Stop condition DETECTED ***/
        SET_USI_TO_DETECT_TWI_START();
    }
    SET_USI_TO_SHIFT_8_ADDRESS_BITS(); /* Wait for TWI address */
}

/*  ______________________________________________________
   |                                                      |
   | USI 4-bit overflow handler (Interrupt-like function) |
   |______________________________________________________|
*/
inline void UsiOverflowHandler(void) {
    switch (device_state) {
        // Check address mode: wait for a START condition and TWI address, if matches this device or
        // is a general call, send ACK (Next -> STATE_SEND_DATA). If it doesn't match, restart USI.
        case STATE_CHECK_ADDRESS: {
            if ((USIDR == 0) || ((USIDR >> 1) == TWI_ADDR)) {
                if (USIDR & 0x01) {             /* If ls-bit = 1, send data to master */
                    ReceiveEvent(rx_byte_count);
                    RequestEvent();
                    device_state = STATE_SEND_DATA;
                } else {                        /* If ls-bit = 0, receive data from master */
                    device_state = STATE_WAIT_DATA_RECEPTION;
                }
                SET_USI_TO_SEND_ACK();
            }
            else {
                SET_USI_TO_WAIT_FOR_TWI_ADDRESS();
            }
            break;
        }
        // Master write data mode: check reply and go
        // to STATE_SEND_DATA if OK, else reset USI
        case STATE_CHECK_ACK_AFTER_SEND_DATA: {
            if (USIDR) {
                // If NACK, the master does not want more data
                SET_USI_TO_WAIT_FOR_TWI_ADDRESS();
                // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                //                                                                 >>
                flags |= (1 << FL_SLOW_OPS); // Enable slow operations in main!      >>
                //                                                                 >> 
                // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                break;
             }
        }
        // From here we just drop straight into STATE_SEND_DATA if the
        // master sent an ACK.
        // Copy data from buffer to USIDR and set USI to shift byte
        // Next -> STATE_WAIT_ACK_AFTER_SEND_DATA
        case STATE_SEND_DATA: {
            // Get data from Buffer
            if (tx_byte_count--) {
                USIDR = tx_buffer[tx_tail];
                tx_tail = (tx_tail + 1) & TWI_TX_BUFFER_MASK;
            } else {
                // The buffer is empty
                SET_USI_TO_WAIT_ACK();  /* This might be necessary, see: http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&p=805227#805227 */
                SET_USI_TO_WAIT_FOR_TWI_ADDRESS();
                return;
            }
            device_state = STATE_WAIT_ACK_AFTER_SEND_DATA;
            SET_USI_TO_SEND_DATA();
            break;
        }
        // Set USI to sample ACK/NACK reply from master
        // Next -> STATE_CHECK_ACK_AFTER_SEND_DATA
        case STATE_WAIT_ACK_AFTER_SEND_DATA: {
            device_state = STATE_CHECK_ACK_AFTER_SEND_DATA;
            SET_USI_TO_WAIT_ACK();
            break;
        }
        // Read data mode: set USI to sample data from master
        // Next -> STATE_RECEIVE_DATA_AND_SEND_ACK
        case STATE_WAIT_DATA_RECEPTION: {
            device_state = STATE_RECEIVE_DATA_AND_SEND_ACK;
            SET_USI_TO_RECEIVE_DATA();
            break;
        }
        // Take data from USIDR and send ACK
        // Next -> STATE_WAIT_DATA_RECEPTION
        case STATE_RECEIVE_DATA_AND_SEND_ACK: {
            // put data into buffer
            // check buffer size
            if (rx_byte_count++ < TWI_RX_BUFFER_SIZE) {
                rx_buffer[rx_head] = USIDR;
                rx_head = (rx_head + 1) & TWI_RX_BUFFER_MASK;
            } else {
                /* Overrun, drop data */
            }
            // Next STATE_WAIT_DATA_RECEPTION
            device_state = STATE_WAIT_DATA_RECEPTION;
            SET_USI_TO_SEND_ACK();
            break;
        }
    }
    USISR |= (1 << USI_OVERFLOW_FLAG); /* Clear the 4-bit counter overflow flag in USI status register to prepare for new interrupts */
}

// -----------------------------------------------------
// USI basic TWI operations functions
// -----------------------------------------------------
inline void SET_USI_TO_WAIT_FOR_TWI_ADDRESS(void) {
    SET_USI_TO_DETECT_TWI_START();
    SET_USI_TO_SHIFT_8_DATA_BITS(); 
}
// -----------------------------------------------------
inline void SET_USI_TO_SEND_DATA(void) {
    SET_USI_SDA_AS_OUTPUT(); /* Drive the SDA line */
    SET_USI_TO_SHIFT_8_DATA_BITS();
}
// -----------------------------------------------------
inline void SET_USI_TO_RECEIVE_DATA(void) {
    SET_USI_SDA_AS_INPUT(); /* Float the SDA line */
    SET_USI_TO_SHIFT_8_DATA_BITS();
}
// -----------------------------------------------------
inline void SET_USI_TO_SEND_ACK(void) {
    USIDR = 0;  /* Clear the USI data register */
    SET_USI_SDA_AS_OUTPUT(); /* Drive the SDA line */
    SET_USI_TO_SHIFT_1_ACK_BIT(); /* Shift-out ACK bit */
}
// -----------------------------------------------------
inline void SET_USI_TO_WAIT_ACK(void) {
    USIDR = 0;  /* Clear the USI data register */
    SET_USI_SDA_AS_INPUT(); /* Float the SDA line */
    SET_USI_TO_SHIFT_1_ACK_BIT(); /* Shift-in ACK bit */
}
// -----------------------------------------------------
inline void SET_USI_TO_DETECT_TWI_START(void) {
    /* Configure USI control register to detect start condition */
    USICR = (1 << TWI_START_COND_INT) | (0 << USI_OVERFLOW_INT) | /* Enable start condition interrupt, disable overflow interrupt */
            (1 << USIWM1) | (0 << USIWM0) | /* Set USI in Two-wire mode, no SCL hold when the 4-bit counter overflows */
            (1 << USICS1) | (0 << USICS0) | (0 << USICLK) | /* Clock Source = External (positive edge) for data register, External (both edges) for 4-Bit counter */
            (0 << USITC); /* No toggle clock-port pin (SCL) */              
}
// -----------------------------------------------------
inline void SET_USI_TO_DETECT_TWI_RESTART(void) {
    /* Configure USI control register to detect RESTART */
    USICR = (1 << TWI_START_COND_INT) | (1 << USI_OVERFLOW_INT) | /* Enable start condition interrupt, disable overflow interrupt */
            (1 << USIWM1) | (1 << USIWM0) | /* Set USI in Two-wire mode, hold SCL low when the 4-bit counter overflows */
            (1 << USICS1) | (0 << USICS0) | (0 << USICLK) | /* Clock Source = External (positive edge) for data register, External (both edges) for 4-Bit counter */
            (0 << USITC); /* No toggle clock-port pin (SCL) */    
}
// -----------------------------------------------------
inline void SET_USI_TO_SHIFT_8_ADDRESS_BITS(void) {
    /* Clear all USI status register interrupt flags to prepare for new start conditions */
    USISR = (1 << TWI_START_COND_FLAG) |
            (1 << USI_OVERFLOW_FLAG) |
            (1 << TWI_STOP_COND_FLAG) |
            (1 << TWI_COLLISION_FLAG) |
            (0x0 << USICNT0); /* Reset status register 4-bit counter to shift 8 bits (data byte to be received) */
}
// -----------------------------------------------------
inline void SET_USI_TO_SHIFT_8_DATA_BITS(void) {
    /* Clear all USI status register interrupt flags, except start condition */
    USISR = (0 << TWI_START_COND_FLAG) |
            (1 << USI_OVERFLOW_FLAG) |
            (1 << TWI_STOP_COND_FLAG) |
            (1 << TWI_COLLISION_FLAG) |
            (0x0 << USICNT0); /* Set status register 4-bit counter to shift 8 bits */    
}
// -----------------------------------------------------
inline void SET_USI_TO_SHIFT_1_ACK_BIT(void) {
    /* Clear all USI status register interrupt flags, except start condition */
    USISR = (0 << TWI_START_COND_FLAG) |
            (1 << USI_OVERFLOW_FLAG) |
            (1 << TWI_STOP_COND_FLAG) |
            (1 << TWI_COLLISION_FLAG) |
            (0x0E << USICNT0); /* Set status register 4-bit counter to shift 1 bit */
}

// -----------------------------------------------------
// USI direction setting functions
// -----------------------------------------------------
inline void SET_USI_SDA_AS_OUTPUT(void) {
    DDR_USI |=  (1 << PORT_USI_SDA);
}
// -----------------------------------------------------
inline void SET_USI_SDA_AS_INPUT(void) {
    DDR_USI &= ~(1 << PORT_USI_SDA);
}
// -----------------------------------------------------
inline void SET_USI_SCL_AS_OUTPUT(void) {
    DDR_USI |=  (1 << PORT_USI_SCL);
}
// -----------------------------------------------------
inline void SET_USI_SCL_AS_INPUT(void) {
    DDR_USI &= ~(1 << PORT_USI_SCL);
}
// -----------------------------------------------------
inline void SET_USI_SDA_AND_SCL_AS_OUTPUT(void) {
    DDR_USI |= (1 << PORT_USI_SDA) | (1 << PORT_USI_SCL);
}
// -----------------------------------------------------
inline void SET_USI_SDA_AND_SCL_AS_INPUT(void) {
    DDR_USI &= ~((1 << PORT_USI_SDA) | (1 << PORT_USI_SCL));
}
