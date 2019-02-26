/*
 *  File: tml-config.h
 *  Project: Timonel - I2C Bootloader for ATtiny85 MCUs
 *  Author: Gustavo Casanova
 *  .......................................................
 *  2018-10-29 gustavo.casanova@nicebots.com
 */

#ifndef _TML_CONFIG_H_
#define _TML_CONFIG_H_

#ifndef __AVR_ATtiny85__
#define __AVR_ATtiny85__
#pragma message "   >>>   Run, Timonel, run!   <<<   "
#endif

/* -------------------------------------- */
/* Timonel settings and optional features */
/* -------------------------------------- */

#define CYCLESTOEXIT    40      /* Loop counter before exit to application if not initialized */

#define LED_UI_PIN      PB1     /* Use >>>PB1<<< to monitor activity. */

/*
   ====== The configuration of the next optional features can be checked
   ====== from the I2C master by using the GETTMNLV command ...
*/

#define ENABLE_LED_UI   true   /* If this is enabled, LED_UI_PIN is used to display Timonel activity. */
                                /* PLEASE DISABLE THIS FOR PRODUCTION! IT COULD ACTIVATE SOMETHING     */
                                /* CONNECTED TO A POWER SOURCE BY ACCIDENT!                            */

#define AUTO_TPL_CALC   false    /* Automatic trampoline calculation & flash. If this is disabled,      */
                                /* the trampoline has to be calculated and written by the I2C master.  */
                                /* Therefore, enabling CMD_STPGADDR becomes mandatory.                 */
                                
#define APP_USE_TPL_PG  false   /* Allow the user appl. to use the trampoline page when AUTO_TPL_CALC  */
                                /* is enabled. This is a safety measure since enabling this takes 2    */
                                /* extra memory pages. In the end, disabling this allows 1 extra page. */
                                /* When AUTO_TPL_CALC is disabled, this option is irrelevant since the */
                                /* duty to write the trampoline page is transferred to the I2C master. */
                                
#define CMD_STPGADDR    true   /* If this is disabled, applications can only be flashed starting      */
                                /* from page 0, this is OK for most applications.                      */
                                /* If this is enabled, Timonel expects STPGADDR before each data page. */
                                /* Enabling this option is MANDATORY when AUTO_TPL_CALC is disabled.   */
                                
#define TWO_STEP_INIT   false   /* If this is enabled, Timonel expects a two-step initialization from  */
                                /* an I2C master for starting. Otherwise, single-step init is enough   */

#define USE_WDT_RESET   true    /* Use watchdog for resetting instead of jumping to TIMONEL_START.     */

#define CHECK_EMPTY_FL  false   /* GETTMNLV will read the first 100 flash memory positions to check if */
                                /* there is an application (or some other data) loaded.                */

#define CMD_READFLASH   true   /* This option enables the READFLSH command. It can be useful for      */
                                /* backing up the flash memory before flashing a new firmware.         */
                                   
/*
   ====== End of feature settings
   ====== shown in the GETTMNLV command.
*/

#define SET_PRESCALER   true    /* If this is enabled, it forces the CPU prescaler division to 1, so   */
                                /* the clock is not divided by 8. This way sets 8 / 16 MHz full speed. */

#define FORCE_ERASE_PG  false   /* If this option is enabled, each flash memory page is erased before  */
                                /* writing new data. Normally, it shouldn't be necessary to enable it. */
                                
/* ---------------------------------------------------------------------------------- */
/* ---   Timonel internal configuration. Do not change anything below this line   --- */
/* ---   unless you're adding a new feature or adapting Timonel to another MCU.   --- */
/* ---------------------------------------------------------------------------------- */

// flash memory definitions
#define PAGE_SIZE       64      /* SPM Flash memory page size */
#define RESET_PAGE      0       /* Interrupt vector table address start location */

// Led UI Port
#define LED_UI_DDR      DDRB    /* >>> WARNING! This is not for use <<< */
#define LED_UI_PORT     PORTB   /* >>> in production!               <<< */

// Operation delays
#define CYCLESTOWAIT    0xFFFF  /* Main loop counter to allow the I2C replies to complete before  */
                                /* performing the selected actions. Also used as LED toggle delay */
                                /* before the I2C master initializes the bootloader.              */

// Timonel ID characters
#define ID_CHAR_1       78      /* N */
#define ID_CHAR_2       66      /* B */
#define ID_CHAR_3       84      /* T */

// I2C TX-RX commands data size
#define TXDATASIZE      10      /* TX data size: always even values, min = 2, max = 10 */
#define RXDATASIZE      8       /* RX data size: always even values, min = 2, max = 8 */

// Status byte
#define ST_INIT_1       0       /* Status Bit 1 (1)  : Two-Step Initialization STEP 1 */
#define ST_INIT_2       1       /* Status Bit 2 (2)  : Two-Step Initialization STEP 2 */
#define ST_DEL_FLASH    2       /* Status Bit 3 (4)  : Delete flash memory            */
#define ST_EXIT_TML     3       /* Status Bit 4 (8)  : Exit Timonel & Run Application */
#define ST_BIT_4        4       /* Status Bit 5 (16) : Not used */
#define ST_BIT_6        5       /* Status Bit 6 (32) : Not used */
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

// Features code calculation for GETTMNLV replies
#if (ENABLE_LED_UI == true)
    #define FT_BIT_0    1
#else
    #define FT_BIT_0    0
#endif
#if (AUTO_TPL_CALC == true)
    #define FT_BIT_1    2
#else
    #define FT_BIT_1    0
#endif
#if (APP_USE_TPL_PG == true)
    #define FT_BIT_2    4
#else
    #define FT_BIT_2    0
#endif
#if (CMD_STPGADDR == true)
    #define FT_BIT_3    8
#else
    #define FT_BIT_3    0
#endif
#if (TWO_STEP_INIT == true)
    #define FT_BIT_4    16
#else
    #define FT_BIT_4    0
#endif
#if (USE_WDT_RESET == true)
    #define FT_BIT_5    32
#else
    #define FT_BIT_5    0
#endif
#if (CHECK_EMPTY_FL == true)
    #define FT_BIT_6    64
#else
    #define FT_BIT_6    0
#endif
#if (CMD_READFLASH == true)
    #define FT_BIT_7    128
#else
    #define FT_BIT_7    0
#endif
#define TML_FEATURES (FT_BIT_7 + FT_BIT_6 + FT_BIT_5 + FT_BIT_4 + FT_BIT_3 + FT_BIT_2 + FT_BIT_1 + FT_BIT_0)

#endif /* _TML_CONFIG_H_ */
