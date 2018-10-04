/*
 *  File: tml-config.h
 *  Project: Timonel - I2C Bootloader for ATtiny85 MCUs
 *  Author: Gustavo Casanova
 *  .......................................................
 *  2018-09-16 gustavo.casanova@nicebots.com
 */

#ifndef _TML_CONFIG_H_
#define _TML_CONFIG_H_

#ifndef __AVR_ATtiny85__
#define __AVR_ATtiny85__
#pragma message "   >>>   Run, Timonel, run!   <<<   "
#endif

/* ------------------------- */
/* Timonel optional features */
/* ------------------------- */
#define ENABLE_LED_UI   false   /* If this is enabled, LED_UI_PIN is used to show Timonel status */
                                /* PLEASE DISABLE THIS FOR PRODUCTION!                           */

#define AUTO_TPL_CALC   true    /* Trampoline auto-calculation & flash. If this is not set, the   */
                                /* trampoline has to be calculated and written by the I2C master. */
                                
#define APP_USE_TPL_PG  false   /* Allow the application to also use the trampoline page: this is more */
                                /* a safety measure since enabling this takes 2 extra memory pagas.    */
                                /* In the end, disabling this allows 1 more application page.          */
                                
#define CMD_STPGADDR    false   /* If this is disabled, applications can only be flashed starting */
                                /* from page 0. This is OK for most applications.                 */
                                
#define TWO_STEP_INIT   false   /* If this is enabled, Timonel expects a two-step initialization from */
                                /* an I2C master for starting. Otherwise, single-step init is enough  */

#define USE_WDT_RESET   false   /* Use watchdog timer for resetting instead of a jump to TIMONEL_START */
                                
#define CYCLESTOEXIT    40      /* Loop counter before exit to application if not initialized */

/* ---------------------------------------------------------------------- */
/* Timonel internal configuration. Do not change anything below this line */
/* ---------------------------------------------------------------------- */

// CPU speed
#ifndef F_CPU
#define F_CPU 8000000UL         /* Default CPU speed for delay.h */
#endif

// flash memory definitions
#define PAGE_SIZE       64      /* SPM Flash memory page size */
#define RESET_PAGE      0       /* Interrupt vector table address start location */

// Led UI
#define LED_UI_PIN      PB1     /* >>> Use PB1 to monitor activity. <<< */
#define LED_UI_DDR      DDRB    /* >>> WARNING! This is not for use <<< */
#define LED_UI_PORT     PORTB   /* >>> in production!               <<< */

// Operation delays
#define CYCLESTOWAIT    0xFFFF  /* Main loop counter to allow the I2C replies to complete. */
                                /* Also used as LED toggle delay before initialization.    */

// Timonel ID characters
#define ID_CHAR_1       78;     /* N */
#define ID_CHAR_2       66;     /* B */
#define ID_CHAR_3       84;     /* T */

// I2C TX-RX commands data size
#define TXDATASIZE      10      /* TX data size: always even values, min = 2, max = 10 */
#define RXDATASIZE      8       /* RX data size: always even values, min = 2, max = 8 */

// Status byte
#define ST_INIT_1       0       /* Status Bit 1 (1)  : Two-Step Initialization STEP 1 */
#define ST_INIT_2       1       /* Status Bit 2 (2)  : Two-Step Initialization STEP 2 */
#define ST_DEL_FLASH    2       /* Status Bit 3 (4)  : Delete flash memory */
//#define ST_APP_READY   3       /* Status Bit 4 (8)  : Application flashed OK, ready to run */
#define ST_BIT_4        3       /* Status Bit 4 (8)  : Not used */
#define ST_EXIT_TML     4       /* Status Bit 5 (16) : Exit Timonel & Run Application */
#define ST_BIT_6        5       /* Status Bit 6 (32) : Not used --- (RTR) Ready to Receive ---*/
#define ST_BIT_7        6       /* Status Bit 7 (64) : Not used */
#define ST_BIT_8        7       /* Status Bit 8 (128): Not used */

// Erase temporary page buffer macro
#define BOOT_TEMP_BUFF_ERASE         (_BV(__SPM_ENABLE) | _BV(CTPB))
#define boot_temp_buff_erase()                   \
(__extension__({                                 \
    __asm__ __volatile__                         \
    (                                            \
        "sts %0, %1\n\t"                         \
        "spm\n\t"                                \
        :                                        \
        : "i" (_SFR_MEM_ADDR(__SPM_REG)),        \
          "r" ((uint8_t)(BOOT_TEMP_BUFF_ERASE))  \
    );                                           \
}))

#endif /* _TML_CONFIG_H_ */

// Features code to reply in GETTMNLV
//#define FEATURES_CODE = 0
//volatile uint8_t featuresCode = 0;

// #if (ENABLE_LED_UI == true)
    // //FEATURES_CODE |= 1
    // featuresCode += 1;
// #endif

// #if (AUTO_TPL_CALC == true)
    // //#FEATURES_CODE |= 2
    // featuresCode += 2;
// #endif

// #if (APP_USE_TPL_PG == true)
    // //#define FEATURES_CODE |= 4
    // featuresCode += 4;
// #endif

// #if (CMD_STPGADDR == true)
    // //#define FEATURES_CODE |= 8
    // featuresCode += 8;
// #endif

// #if (TWO_STEP_INIT == true)
    // //#define FEATURES_CODE |= 16
    // featuresCode += 16;
// #endif

// #if (USE_WDT_RESET == true)
    // //#define FEATURES_CODE |= 32
    // featuresCode += 32;
// #endif

