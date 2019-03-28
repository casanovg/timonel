/*
 *  Timonel - TWI Bootloader for TinyX5 MCUs
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: timonel.h (Main bootloader headers)
 *  ........................................... 
 *  Version: 1.2 / 2019-03-15
 *  gustavo.casanova@nicebots.com
 *  ...........................................
 */
 
/*
 *****************************************************************************
 * Please do NOT modify this file directly, change "tml-config.mak" instead! *
 *****************************************************************************
*/

#ifndef _TML_CONFIG_H_
#define _TML_CONFIG_H_

#ifndef __AVR_ATtiny85__
#define __AVR_ATtiny85__
#pragma message "   >>>   Run, Timonel, run!   <<<   "
#endif

/* Includes */
#include <avr/boot.h>
#include <avr/wdt.h>
#include <stdbool.h>

#include "../nb-libs-as1/nb-usitwisl-if.h"
#include "../nb-libs-as1/nb-twi-cmd.h"

/* -------------------------------------- */
/* Timonel settings and optional features */
/* -------------------------------------- */

/* ====== [   The configuration of the next optional features can be checked   ] ====== */
/* ====== [   from the I2C master by using the GETTMNLV command. Please do NOT ] ====== */        
/* VVVVVV [   modify this options directly, change "tml-config.mak" instead!   ] VVVVVV */

#ifndef ENABLE_LED_UI           /* If this is enabled, LED_UI_PIN is used to display Timonel activity. */
#define ENABLE_LED_UI   false   /* PLEASE DISABLE THIS FOR PRODUCTION! IT COULD ACTIVATE SOMETHING     */
#endif /* ENABLE_LED_UI */      /* CONNECTED TO A POWER SOURCE BY ACCIDENT!                            */
           
#ifndef AUTO_TPL_CALC           /* Automatic trampoline calculation & flash. If this is disabled,      */
#define AUTO_TPL_CALC   true    /* the trampoline has to be calculated and written by the I2C master.  */
#endif /* AUTO_TPL_CALC */      /* Therefore, enabling CMD_STPGADDR becomes mandatory.                 */
                                
#ifndef APP_USE_TPL_PG          /* Allow the user appl. to use the trampoline page when AUTO_TPL_CALC  */
#define APP_USE_TPL_PG  false   /* is enabled. This is a safety measure since enabling this takes 2    */
#endif /* APP_USE_TPL_PG */     /* extra memory pages. In the end, disabling this allows 1 extra page. */
                                /* When AUTO_TPL_CALC is disabled, this option is irrelevant since the */
                                /* duty to write the trampoline page is transferred to the I2C master. */
                                
#ifndef CMD_STPGADDR            /* If this is disabled, applications can only be flashed starting      */
#define CMD_STPGADDR    false   /* from page 0, this is OK for most applications.                      */
#endif /* CMD_STPGADDR */       /* If this is enabled, Timonel expects STPGADDR before each data page. */
                                /* Enabling this option is MANDATORY when AUTO_TPL_CALC is disabled.   */
                                
#ifndef TWO_STEP_INIT           /* If this is enabled, Timonel expects a two-step initialization from  */
#define TWO_STEP_INIT   false   /* an I2C master for starting. Otherwise, single-step init is enough   */
#endif /* TWO_STEP_INIT */

#ifndef USE_WDT_RESET           /* Use watchdog for resetting instead of jumping to TIMONEL_START.     */
#define USE_WDT_RESET   true    
#endif /* USE_WDT_RESET */

#ifndef CHECK_EMPTY_FL          /* GETTMNLV will read the first 100 flash memory positions to check if */
#define CHECK_EMPTY_FL  false   /* there is an application (or some other data) loaded.                */
#endif /* CHECK_EMPTY_FL */

#ifndef CMD_READFLASH           /* This option enables the READFLSH command. It can be useful for      */
#define CMD_READFLASH   false   /* backing up the flash memory before flashing a new firmware.         */
#endif /* CMD_READFLASH */                                   

/* ^^^^^^ [   ..............  End of feature settings shown  ...............   ] ^^^^^^ */
/* ====== [                   in the GETTMNLV command.                         ] ====== */
/* ====== [   ..............................................................   ] ====== */

#ifndef CYCLESTOEXIT
#define CYCLESTOEXIT    40      /* Loop counter before exit to application if not initialized */
#endif /* CYCLESTOEXIT */

#ifndef LED_UI_PIN
#define LED_UI_PIN      PB1     /* Use >>>PB1<<< to monitor activity. */
#endif /* LED_UI_PIN */


#ifndef SET_PRESCALER           /* If this is enabled, it forces the CPU prescaler division to 1, so   */
#define SET_PRESCALER   true    /* the clock is not divided by 8. This way sets 8 / 16 MHz full speed. */
#endif /* SET_PRESCALER */

#ifndef FORCE_ERASE_PG          /* If this option is enabled, each flash memory page is erased before  */
#define FORCE_ERASE_PG  false   /* writing new data. Normally, it shouldn't be necessary to enable it. */
#endif /* FORCE_ERASE_PG */

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

// TWI Commands Xmit data block size
#define MST_DATA_SIZE   8       /* Master-to-Slave Xmit data block size: always even values, min = 2, max = 8 */
#define SLV_DATA_SIZE   8       /* Slave-to-Master Xmit data block size: always even values, min = 2, max = 8 */

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
