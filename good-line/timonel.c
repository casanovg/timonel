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
 *  Version: 1.3 "Sandra" / 2019-04-23 (GOOD-LINE)
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
                                
#if ((MST_DATA_SIZE > 8) || ((SLV_DATA_SIZE > 8)))
    #pragma GCC warning "Don't set transmission data size too high to avoid affecting the TWI reliability!"
#endif

#if ((CYCLESTOEXIT > 0) && (CYCLESTOEXIT < 20))
    #pragma GCC warning "Do not set CYCLESTOEXIT too low, it could make difficult for TWI master to initialize on time!"
#endif

// Type definitions
/* Pointer-to-address type */
typedef void (* const fptr_t)(void);
/* TWI driver operational modes */
typedef enum {
	STATE_CHECK_ADDRESS,
    STATE_SEND_DATA,
    STATE_WAIT_ACK_AFTER_SEND_DATA,
    STATE_CHECK_ACK_AFTER_SEND_DATA,
    STATE_WAIT_DATA_RECEPTION,
    STATE_RECEIVE_DATA_AND_SEND_ACK   
} OperationalState;

// Global variables
uint8_t command[(MST_DATA_SIZE * 2) + 2] = { 0 }; /* Command received from TWI master */
uint8_t flags = 0;                             /* Bit: 8,7,6,5: Not used; 4: exit; 3: delete flash; 2, 1: initialized */
uint16_t flashPageAddr = 0x0000;               /* Flash memory page address */
uint8_t pageIX = 0;                            /* Flash memory page index */
uint8_t rx_buffer[TWI_RX_BUFFER_SIZE];
uint8_t rx_head;
uint8_t rx_tail;
uint8_t rx_count;
uint8_t tx_buffer[TWI_TX_BUFFER_SIZE];
uint8_t tx_head;
uint8_t tx_tail;
uint8_t tx_count;
OperationalState device_state;

#if AUTO_TPL_CALC
uint8_t appResetLSB = 0xFF;                    /* Application first byte */
uint8_t appResetMSB = 0xFF;                    /* Application second byte */
#endif /* AUTO_TPL_CALC */

// Jump to trampoline
static const fptr_t RunApplication = (const fptr_t)((TIMONEL_START - 2) / 2);
#if !(USE_WDT_RESET)
// Restart this bootloader
static const fptr_t RestartTimonel = (const fptr_t)(TIMONEL_START / 2);
#endif /* !USE_WDT_RESET */

// Prototypes
inline void ReceiveEvent(uint8_t) __attribute__((always_inline));
static inline void RequestEvent(void) __attribute__((always_inline));

uint8_t UsiTwiReceiveByte(void);
static void UsiTwiTransmitByte(uint8_t);

// Helper function prototypes
static inline void UsiTwiSlaveInit(void) __attribute__((always_inline));
static inline bool UsiTwiDataInTransmitBuffer(void) __attribute__((always_inline));
static inline void FlushTwiBuffers(void) __attribute__((always_inline));

// I2C handlers prototypes (These functions replace the USI hardware interrupts)
static inline void UsiStartHandler(void) __attribute__((always_inline));
static inline void UsiOverflowHandler(void) __attribute__((always_inline));

// USI direction setting prototypes
static inline void SET_USI_SDA_AS_OUTPUT() __attribute__((always_inline));
static inline void SET_USI_SDA_AS_INPUT() __attribute__((always_inline));
static inline void SET_USI_SCL_AS_OUTPUT() __attribute__((always_inline));
static inline void SET_USI_SCL_AS_INPUT() __attribute__((always_inline));
static inline void SET_USI_SDA_AND_SCL_AS_OUTPUT() __attribute__((always_inline));
static inline void SET_USI_SDA_AND_SCL_AS_INPUT() __attribute__((always_inline));

// USI basic TWI operations prototypes
static inline void SET_USI_TO_SEND_ACK() __attribute__((always_inline));
static inline void SET_USI_TO_WAIT_ACK() __attribute__((always_inline));
static inline void SET_USI_TO_WAIT_FOR_START_COND_AND_ADDRESS() __attribute__((always_inline));
static inline void SET_USI_TO_SEND_DATA() __attribute__((always_inline));
static inline void SET_USI_TO_RECEIVE_DATA() __attribute__((always_inline));

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
	UsiTwiSlaveInit();              		/* Initialize TWI driver */
    __SPM_REG = (_BV(CTPB) | \
                 _BV(__SPM_ENABLE));        /* Clear temporary page buffer */
    asm volatile("spm");
    //uint8_t exitDly = CYCLESTOEXIT;         /* Delay to exit bootloader and run the application if not initialized */
    uint16_t exitDly = 0xFFF;         /* Delay to exit bootloader and run the application if not initialized */
    uint8_t boogie = 0xFF;
    
    /*  ___________________
       |                   | 
       |     Main Loop     |
       |___________________|
    */
    for (;;) {
        /* .....................................................
           . TWI Interrupt Emulation ......................... .
           . Check the USI Status Register to verify whether   .
           . a TWI start condition handler should be launched  .
           .....................................................
        */
        if ((USISR & (1 << TWI_START_COND_FLAG)) && (USICR & (1 << TWI_START_COND_INT))) {
            UsiStartHandler();                      /* If so, run the USI start handler ... */
        } 		
        /* .....................................................
           . TWI Interrupt Emulation ......................... .
           . Check the USI Status Register to verify whether   .
           . a USI counter overflow handler should be launched .
           .....................................................
        */
        if ((USISR & (1 << USI_OVERFLOW_FLAG)) && (USICR & (1 << USI_OVERFLOW_INT))) {
            UsiOverflowHandler(); /* If so, run the USI overflow handler ... */
        }
#if !(TWO_STEP_INIT)
        if ((flags & (1 << FL_INIT_1)) != (1 << FL_INIT_1)) {
#endif /* !TWO_STEP_INIT */
#if TWO_STEP_INIT
        if ((flags & ((1 << FL_INIT_1) + (1 << FL_INIT_2))) != ((1 << FL_INIT_1) + (1 << FL_INIT_2))) {
#endif /* TWO_STEP_INIT */
            /*  ____________________________
               |                            |
               | Bootloader not initialized |
               |____________________________|
            */
#if ENABLE_LED_UI               
            LED_UI_PORT ^= (1 << LED_UI_PIN);   /* Blinks on each main loop pass at CYCLESTOWAIT intervals */
#endif /* ENABLE_LED_UI */
            if (boogie-- == 0) {
                if (exitDly-- == 0) {
                    RunApplication();               /* Count from CYCLESTOEXIT to 0, then exit to the application */
                }
            }
        }
        else {
            /*  _______________________________
               |                               |
               |   Bootloader initialized !!!  |
               |_______________________________|
            */            
            //if (enable_slow_ops == true) {
            //    enable_slow_ops = false;
            if ((flags & (1 << FL_EN_SLOW_OPS)) == (1 << FL_EN_SLOW_OPS)) {
                flags &= ~(1 << FL_EN_SLOW_OPS);
                // =================================================
                // = Exit bootloader and run application (Slow Op) =
                // =================================================
                if ((flags & (1 << FL_EXIT_TML)) == (1 << FL_EXIT_TML)) {
                    asm volatile("cbr r31, 0x80");          /* Clear bit 7 of r31 */
                    RunApplication();                       /* Exit to the application */
                }
                // ==================================================
                // = Delete application from flash memory (Slow Op) =
                // ==================================================
                if ((flags & (1 << FL_DEL_FLASH)) == (1 << FL_DEL_FLASH)) {
#if ENABLE_LED_UI                   
                    LED_UI_PORT |= (1 << LED_UI_PIN);       /* Turn led on to indicate erasing ... */
#endif /* ENABLE_LED_UI */
                    uint16_t pageAddress = TIMONEL_START;   /* Erase flash ... */
                    while (pageAddress != RESET_PAGE) {
                        pageAddress -= PAGE_SIZE;
                        boot_page_erase(pageAddress);
                    }
#if !(USE_WDT_RESET)
                    RestartTimonel();                       /* Restart the bootloader by jumping to Timonel start */
#else
                    wdt_enable(WDTO_15MS);                  /* Restart the bootloader by activating the watchdog timer */
                    for (;;) {};
#endif /* !USE_WDT_RESET */                    
                }
#if (APP_USE_TPL_PG || !(AUTO_TPL_CALC))
                // ========================================================================
                // = Write received page to flash memory and prepare to receive a new one =
                // ========================================================================    
                if ((pageIX == PAGE_SIZE) & (flashPageAddr < TIMONEL_START)) {
#else
                if ((pageIX == PAGE_SIZE) & (flashPageAddr < TIMONEL_START - PAGE_SIZE)) {
#endif /* APP_USE_TPL_PG || !(AUTO_TPL_CALC) */
#if ENABLE_LED_UI
                    LED_UI_PORT ^= (1 << LED_UI_PIN);   /* Turn led on and off to indicate writing ... */
#endif /* ENABLE_LED_UI */
#if FORCE_ERASE_PG
                    boot_page_erase(flashPageAddr);
#endif /* FORCE_ERASE_PG */                    
                    boot_page_write(flashPageAddr);
#if AUTO_TPL_CALC
                    if (flashPageAddr == RESET_PAGE) {    /* Calculate and write trampoline */
                        uint16_t tpl = (((~((TIMONEL_START >> 1) - ((((appResetMSB << 8) | appResetLSB) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
                        for (int i = 0; i < PAGE_SIZE - 2; i += 2) {
                            boot_page_fill((TIMONEL_START - PAGE_SIZE) + i, 0xFFFF);
                        }
                        boot_page_fill((TIMONEL_START - 2), tpl);
                        boot_page_write(TIMONEL_START - PAGE_SIZE);                        
                    }
#if APP_USE_TPL_PG
                    if ((flashPageAddr) == (TIMONEL_START - PAGE_SIZE)) {
                        uint16_t tpl = (((~((TIMONEL_START >> 1) - ((((appResetMSB << 8) | appResetLSB) + 1) & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
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
                    flashPageAddr += PAGE_SIZE;
#endif /* !CMD_STPGADDR */
                    pageIX = 0;
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
        command[i] = UsiTwiReceiveByte();                           /* Store the data sent by the TWI master in the data buffer */
    }
}

/*  ________________________
   |                        |
   | TWI data request event |
   |________________________|
*/
inline void RequestEvent(void) {
    //uint8_t opCodeAck = ~command[0];                                /* Command code reply => Command Bitwise "Not" */
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
            uint8_t reply[GETTMNLV_RPLYLN] = { 0 };
            reply[0] = ACKTMNLV;
            reply[1] = ID_CHAR_3;                                   /* T */            
            reply[2] = TIMONEL_VER_MJR;                             /* Major version number */
            reply[3] = TIMONEL_VER_MNR;                             /* Minor version number */
            reply[4] = TML_FEATURES;                                /* Optional features */
            reply[5] = ((TIMONEL_START & 0xFF00) >> 8);             /* Start address MSB */
            reply[6] = (TIMONEL_START & 0xFF);                      /* Start address LSB */
            reply[7] = (*flashAddr & 0xFF);                         /* Trampoline second byte (MSB) */
            reply[8] = (*(--flashAddr) & 0xFF);                     /* Trampoline first byte (LSB) */
#if CHECK_EMPTY_FL
            for (uint16_t mPos = 0; mPos < 100; mPos++) {           /* Check the first 100 memory positions to determine if  */
                flashAddr = (void *)(mPos);                         /* there is an application (or some other data) loaded.  */
                reply[9] += (uint8_t)~(*flashAddr);                    
            }
#endif /* CHECK_EMPTY_FL */
#if !(TWO_STEP_INIT)
            //flags |= (1 << (FL_INIT_1)) | (1 << (FL_INIT_2));     /* Single-step init */
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
            //break;
            return;
        }
        // ******************
        // * EXITTMNL Reply *
        // ******************
        case EXITTMNL: {
            UsiTwiTransmitByte(ACKEXITT);
            flags |= (1 << FL_EXIT_TML);
            //break;
            return;
        }
        // ******************
        // * DELFLASH Reply *
        // ******************
        case DELFLASH: {
            UsiTwiTransmitByte(ACKDELFL);
            flags |= (1 << FL_DEL_FLASH);
            //break;
            return;
        }
#if (CMD_STPGADDR || !(AUTO_TPL_CALC))
        // ******************
        // * STPGADDR Reply *
        // ******************
        case STPGADDR: {
            #define STPGADDR_RPLYLN 2
            uint8_t reply[STPGADDR_RPLYLN] = { 0 };
            flashPageAddr = ((command[1] << 8) + command[2]);       /* Sets the flash memory page base address */
            flashPageAddr &= ~(PAGE_SIZE - 1);                      /* Keep only pages' base addresses */
            reply[0] = AKPGADDR;
            reply[1] = (uint8_t)(command[1] + command[2]);             /* Returns the sum of MSB and LSB of the page address */
            for (uint8_t i = 0; i < STPGADDR_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            //break;
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
            if ((flashPageAddr + pageIX) == RESET_PAGE) {
#if AUTO_TPL_CALC
                appResetLSB = command[1];
                appResetMSB = command[2];
#endif /* AUTO_TPL_CALC */
                // Modify the reset vector to point to this bootloader.
                // WARNING: This only works when CMD_STPGADDR is disabled. If CMD_STPGADDR is enabled,
                // the reset vector modification MUST BE done by the TWI master's upload program.
                // Otherwise, Timonel won't have the execution control after power on reset.
                boot_page_fill((RESET_PAGE), (0xC000 + ((TIMONEL_START / 2) - 1)));
                reply[1] += (uint8_t)((command[2]) + command[1]);    	/* Reply checksum accumulator */
                pageIX += 2;
                for (uint8_t i = 3; i < (MST_DATA_SIZE + 1); i += 2) {
                    boot_page_fill((flashPageAddr + pageIX), ((command[i + 1] << 8) | command[i]));
                    reply[1] += (uint8_t)((command[i + 1]) + command[i]);
                    pageIX += 2;
                }                
            }
            else {
                for (uint8_t i = 1; i < (MST_DATA_SIZE + 1); i += 2) {
                    boot_page_fill((flashPageAddr + pageIX), ((command[i + 1] << 8) | command[i]));
                    reply[1] += (uint8_t)((command[i + 1]) + command[i]);
                    pageIX += 2;
                }
            }
            if ((reply[1] != command[MST_DATA_SIZE + 1]) || (pageIX > PAGE_SIZE)) {
                flags |= (1 << FL_DEL_FLASH);            	/* If checksums don't match, safety payload deletion ... */
                reply[1] = 0;
            }
            for (uint8_t i = 0; i < WRITPAGE_RPLYLN; i++) {
                UsiTwiTransmitByte(reply[i]);
            }
            //break;
            return;
        }
#if CMD_READFLASH
        // ******************
        // * READFLSH Reply *
        // ******************
        case READFLSH: {
            const uint8_t ackLng = (command[3] + 2);                   /* Fourth byte received determines the size of reply data */
            uint8_t reply[ackLng];
            if ((command[3] >= 1) & (command[3] <= (SLV_DATA_SIZE + 2) * 2) & ((uint8_t)(command[0] + command[1] + command[2] + command[3]) == command[4])) {
                reply[0] = ACKRDFSH;
                flashPageAddr = ((command[1] << 8) + command[2]);   /* Sets the flash memory page base address */
                reply[ackLng - 1] = 0;                              /* Checksum initialization */
                const __flash unsigned char * flashAddr;
                flashAddr = (void *)flashPageAddr; 
                for (uint8_t i = 1; i < command[3] + 1; i++) {
                    reply[i] = (*(flashAddr++) & 0xFF);             /* Actual flash data */
                    reply[ackLng - 1] += (uint8_t)(reply[i]);          /* Checksum accumulator to be sent in the last byte of the reply */
                }                
                for (uint8_t i = 0; i < ackLng; i++) {
                    UsiTwiTransmitByte(reply[i]);
                }
            }
            else {
                UsiTwiTransmitByte(UNKNOWNC);                       /* Incorrect operand value received */
            }
            //break;
            return;
        }   
#endif /* CMD_READFLASH */
#if TWO_STEP_INIT
        // ******************
        // * INITSOFT Reply *
        // ******************
        case INITSOFT: {
            flags |= (1 << FL_INIT_1);                     /* Two-step init step 1: receive INITSOFT command */
            UsiTwiTransmitByte(ACKINITS);
            //break;
            return;
        }
#endif /* TWO_STEP_INIT */
        default: {
            UsiTwiTransmitByte(UNKNOWNC);
            //break;
            return;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
////////////       ALL USI TWI DRIVER CODE BELOW THIS LINE       ////////////
/////////////////////////////////////////////////////////////////////////////

// Function FlushTwiBuffers
inline void FlushTwiBuffers(void) {
    rx_tail = 0;
    rx_head = 0;
    rx_count = 0;
    tx_tail = 0;
    tx_head = 0;
    tx_count = 0;
}

// Function UsiTwiSlaveInit
inline void UsiTwiSlaveInit(void) {
	/* In Two Wire mode (USIWM1, USIWM0 = 1X), the slave USI will pull SCL
       low when a start condition is detected or a counter overflow (only
       for USIWM1, USIWM0 = 11).  This inserts a wait state.  SCL is released
       by the ISRs (USI_START_vect and USI_OVERFLOW_vect).
	*/
    FlushTwiBuffers();
	SET_USI_SDA_AND_SCL_AS_OUTPUT(); /* Set SCL and SDA as output */
    PORT_USI |= (1 << PORT_USI_SCL); /* Set SCL high */
    PORT_USI |= (1 << PORT_USI_SDA); /* Set SDA high */
	SET_USI_SDA_AS_INPUT(); 		 /* Set SDA as input */
	SET_USI_TO_WAIT_FOR_START_COND_AND_ADDRESS(); /* Wait for TWI start condition and address from master */
}

// Function UsiTwiDataInTransmitBuffer
inline bool UsiTwiDataInTransmitBuffer(void) { 
    return tx_count; /* Return 0 (false) if the receive buffer is empty */
}

// Function UsiTwiTransmitByte
void UsiTwiTransmitByte(uint8_t data_byte) {
    while (tx_count++ == TWI_TX_BUFFER_SIZE) {
		/* Wait until there is free space in the TX buffer */
    };
    tx_buffer[tx_head] = data_byte; /* Write the data byte into the TX  buffer */
    tx_head = (tx_head + 1) & TWI_TX_BUFFER_MASK;
}

// Function UsiTwiReceiveByte
uint8_t UsiTwiReceiveByte(void) {
    uint8_t received_byte;
    while (!rx_count--) {
        /* Wait until a byte is received into the RX buffer */
    };
    received_byte = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) & TWI_RX_BUFFER_MASK; /* Calculate the buffer index */
    return received_byte; /* Return the received byte from the buffer */
}

// Function UsiStartHandler (Interrupt-like handler function)
inline void UsiStartHandler() {
    device_state = STATE_CHECK_ADDRESS; /* Set default starting conditions for a new TWI package */
	SET_USI_SDA_AS_INPUT(); /* Float the SDA line */
    while ((PIN_USI & (1 << PORT_USI_SCL)) && (!(PIN_USI & (1 << PORT_USI_SDA)))) {
		/* Wait for SCL to go low to ensure the start condition has completed (the
		   start detector will hold SCL low ).
		*/
	}
	// If a stop condition arises then leave this function to prevent waiting forever.
	// Don't use USISR to test for stop condition as in application note AVR312
	// because the stop condition flag is going to be set from the last TWI sequence.
	if (!(PIN_USI & (1 << PIN_USI_SDA))) {	/*** Stop condition NOT DETECTED ***/
		// Keep start condition interrupt enabled to detect RESTART
		USICR = (1 << TWI_START_COND_INT) | (1 << USI_OVERFLOW_INT) | /* Enable start condition interrupt, disable overflow interrupt */
				(1 << USIWM1) | (1 << USIWM0) | /* Set USI in Two-wire mode, hold SCL low when the 4-bit counter overflows */
				(1 << USICS1) | (0 << USICS0) | (0 << USICLK) | /* Clock Source = External (positive edge) for data register, External (both edges) for 4-Bit counter */
				(0 << USITC); /* No toggle clock-port pin (SCL) */
    } else { 								/*** Stop condition DETECTED ***/
		/* Configure USI control register */
		USICR = (1 << TWI_START_COND_INT) | (0 << USI_OVERFLOW_INT) | /* Enable start condition interrupt, disable overflow interrupt */
				(1 << USIWM1) | (0 << USIWM0) | /* Set USI in Two-wire mode, no SCL hold when the 4-bit counter overflows */
				(1 << USICS1) | (0 << USICS0) | (0 << USICLK) | /* Clock Source = External (positive edge) for data register, External (both edges) for 4-Bit counter */
				(0 << USITC); /* No toggle clock-port pin (SCL) */				
    }
	/* Clear all USI status register interrupt flags to prepare for new start conditions */
	USISR = (1 << TWI_START_COND_FLAG) |
			(1 << USI_OVERFLOW_FLAG) |
			(1 << TWI_STOP_COND_FLAG) |
			(1 << TWI_COLLISION_FLAG) |
			(0x0 << USICNT0); /* Reset status register 4-bit counter to shift 8 bits (data byte to be received) */  
}

// Function UsiOverflowHandler (Interrupt-like handler function)
inline void UsiOverflowHandler() {
    switch (device_state) {
        // Check address mode: check received address and send ACK (and next STATE_SEND_DATA) if OK,
        // else reset USI
        case STATE_CHECK_ADDRESS: {
            if ((USIDR == 0) || ((USIDR >> 1) == TWI_ADDR)) {
                if (USIDR & 0x01) {             /* If lsbit = 1: Send data to master */
                    //DATA_REQUESTED_BY_MASTER_CALLBACK();
                    ReceiveEvent(rx_count);
                    RequestEvent();
                    device_state = STATE_SEND_DATA;
                } else {                        /* If lsbit = 0: Receive data from master */
                    device_state = STATE_WAIT_DATA_RECEPTION;
                }
                SET_USI_TO_SEND_ACK();
            }
            else {
                SET_USI_TO_WAIT_FOR_START_COND_AND_ADDRESS();
            }
            break;
        }
        // Master write data mode: check reply and goto STATE_SEND_DATA if OK,
        // else reset USI
        case STATE_CHECK_ACK_AFTER_SEND_DATA: {
            if (USIDR) {
                // If NACK, the master does not want more data
                SET_USI_TO_WAIT_FOR_START_COND_AND_ADDRESS();
                // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                //                                                                 >>
                flags |= (1 << FL_EN_SLOW_OPS); // Enable slow operations in main!   >>
                //                                                                 >> 
                // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                break;
             }
        }
        // From here we just drop straight into STATE_SEND_DATA if the
        // master sent an ACK.
        // Copy data from buffer to USIDR and set USI to shift byte
        // Next STATE_WAIT_ACK_AFTER_SEND_DATA
        case STATE_SEND_DATA: {
            // Get data from Buffer
            if (tx_count--) {
                USIDR = tx_buffer[tx_tail];
                tx_tail = (tx_tail + 1) & TWI_TX_BUFFER_MASK;
            } else {
                // The buffer is empty
                SET_USI_TO_WAIT_ACK();  // This might be necessary sometimes see http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&p=805227#805227
                SET_USI_TO_WAIT_FOR_START_COND_AND_ADDRESS();
                return;
            }
            device_state = STATE_WAIT_ACK_AFTER_SEND_DATA;
            SET_USI_TO_SEND_DATA();
            break;
        }
        // Set USI to sample ACK/NACK reply from master
        // Next STATE_CHECK_ACK_AFTER_SEND_DATA
        case STATE_WAIT_ACK_AFTER_SEND_DATA: {
            device_state = STATE_CHECK_ACK_AFTER_SEND_DATA;
            SET_USI_TO_WAIT_ACK();
            break;
        }
        // Read data mode: set USI to sample data from master, next
        // STATE_RECEIVE_DATA_AND_SEND_ACK
        case STATE_WAIT_DATA_RECEPTION: {
            device_state = STATE_RECEIVE_DATA_AND_SEND_ACK;
            SET_USI_TO_RECEIVE_DATA();
            break;
        }
        // Take data from USIDR and send ACK
        // Next STATE_WAIT_DATA_RECEPTION
        case STATE_RECEIVE_DATA_AND_SEND_ACK: {
            // put data into buffer
            // check buffer size
            if (rx_count++ < TWI_RX_BUFFER_SIZE) {
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
// USI direction setting functions
// -----------------------------------------------------
inline void SET_USI_SDA_AS_OUTPUT() {
	DDR_USI |=  (1 << PORT_USI_SDA);
}
// -----------------------------------------------------
inline void SET_USI_SDA_AS_INPUT() {
	DDR_USI &= ~(1 << PORT_USI_SDA);
}
// -----------------------------------------------------
inline void SET_USI_SCL_AS_OUTPUT() {
	DDR_USI |=  (1 << PORT_USI_SCL);
}
// -----------------------------------------------------
inline void SET_USI_SCL_AS_INPUT() {
	DDR_USI &= ~(1 << PORT_USI_SCL);
}
// -----------------------------------------------------
inline void SET_USI_SDA_AND_SCL_AS_OUTPUT() {
	DDR_USI |= (1 << PORT_USI_SDA) | (1 << PORT_USI_SCL);
}
// -----------------------------------------------------
inline void SET_USI_SDA_AND_SCL_AS_INPUT() {
	DDR_USI &= ~((1 << PORT_USI_SDA) | (1 << PORT_USI_SCL));
}

// -----------------------------------------------------
// USI basic TWI operations functions
// -----------------------------------------------------
inline void SET_USI_TO_SEND_ACK() {
	USIDR = 0;	/* Set the USI data register for sending an ACK (TWI ACK = low) */
	SET_USI_SDA_AS_OUTPUT(); /* Drive the SDA line */
	/* Clear all USI status register interrupt flags, except start condition (so SCL clock line is NOT released???) */
	USISR = (0 << TWI_START_COND_FLAG) |
			(1 << USI_OVERFLOW_FLAG) |
			(1 << TWI_STOP_COND_FLAG) |
			(1 << TWI_COLLISION_FLAG) |
			(0x0E << USICNT0); /* Set status register 4-bit counter to shift 1 bit (ACK bit) */
}
// -----------------------------------------------------
inline void SET_USI_TO_WAIT_ACK() {
	USIDR = 0;	/* Clear the USI data register to receive an ACK */
	SET_USI_SDA_AS_INPUT(); /* Float the SDA line */
	/* Clear all USI status register interrupt flags, except start condition */
	USISR = (0 << TWI_START_COND_FLAG) |
			(1 << USI_OVERFLOW_FLAG) |
			(1 << TWI_STOP_COND_FLAG) |
			(1 << TWI_COLLISION_FLAG) |
			(0x0E << USICNT0); /* Set status register 4-bit counter to shift 1 bit (ACK bit) */	
}
// -----------------------------------------------------
inline void SET_USI_TO_WAIT_FOR_START_COND_AND_ADDRESS() {
	/* Configure USI control register */
	USICR = (1 << TWI_START_COND_INT) | (0 << USI_OVERFLOW_INT) | /* Enable start condition interrupt, disable overflow interrupt */
		    (1 << USIWM1) | (0 << USIWM0) | /* Set USI in Two-wire mode, no SCL hold when the 4-bit counter overflows */
			(1 << USICS1) | (0 << USICS0) | (0 << USICLK) | /* Clock Source = External (positive edge) for data register, External (both edges) for 4-Bit counter */
			(0 << USITC); /* No toggle clock-port pin (SCL) */
	/* Clear all USI status register interrupt flags, except start condition */
	USISR = (0 << TWI_START_COND_FLAG) |
			(1 << USI_OVERFLOW_FLAG) |
			(1 << TWI_STOP_COND_FLAG) |
			(1 << TWI_COLLISION_FLAG) |
			(0x0 << USICNT0); /* Reset status register 4-bit counter to shift 8 bits (address byte to be received) */	 		
}
// -----------------------------------------------------
inline void SET_USI_TO_SEND_DATA() {
	SET_USI_SDA_AS_OUTPUT(); /* Drive the SDA line */
	/* Clear all USI status register interrupt flags, except start condition */
	USISR = (0 << TWI_START_COND_FLAG) |
			(1 << USI_OVERFLOW_FLAG) |
			(1 << TWI_STOP_COND_FLAG) |
			(1 << TWI_COLLISION_FLAG) |
			(0x0 << USICNT0); /* Reset status register 4-bit counter to shift 8 bits (data byte to be transmitted) */		
}
// -----------------------------------------------------
inline void SET_USI_TO_RECEIVE_DATA() {
	SET_USI_SDA_AS_INPUT(); /* Float the SDA line */
	/* Clear all USI status register interrupt flags, except start condition */
	USISR = (0 << TWI_START_COND_FLAG) |
			(1 << USI_OVERFLOW_FLAG) |
			(1 << TWI_STOP_COND_FLAG) |
			(1 << TWI_COLLISION_FLAG) |
			(0x0 << USICNT0); /* Reset status register 4-bit counter to shift 8 bits (data byte to be received) */		
	
}
