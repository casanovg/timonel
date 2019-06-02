/*
 *  Timonel - TWI Bootloader for TinyX5 MCUs
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: timonel.h (Main bootloader headers)
 *  ........................................... 
 *  Version: 1.3 "Sandra" / 2019-06-06
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
#include <avr/interrupt.h>
#include "../nb-libs/cmd/nb-twi-cmd.h"

/* ====== [   The configuration of the next optional features can be checked   ] ====== */        
/* VVVVVV [   from the I2C master by using the GETTMNLV command.               ] VVVVVV */

#ifndef ENABLE_LED_UI               /* If this is enabled, LED_UI_PIN is used to display Timonel activity. */
#define ENABLE_LED_UI   false       /* PLEASE DISABLE THIS FOR PRODUCTION! IT COULD ACTIVATE SOMETHING     */
#endif /* ENABLE_LED_UI */          /* CONNECTED TO A POWER SOURCE BY ACCIDENT!                            */
           
#ifndef AUTO_PAGE_ADDR              /* Automatic page and trampoline address calculation. If this option   */
#define AUTO_PAGE_ADDR  true        /* is disabled, the trampoline has to be calculated and written by the */
#endif /* AUTO_PAGE_ADDR */         /* I2C master. Therefore, enabling CMD_SETPGADDR becomes mandatory.    */
                                
#ifndef APP_USE_TPL_PG              /* Allow the user appl. to use the trampoline page when AUTO_PAGE_ADDR */
#define APP_USE_TPL_PG  false       /* is enabled. This is a safety measure since enabling this takes 2    */
#endif /* APP_USE_TPL_PG */         /* extra memory pages. In the end, disabling this allows 1 extra page. */
                                    /* If AUTO_PAGE_ADDR is disabled, this option is irrelevant since the  */
                                    /* trampoline page writing is to be made by the I2C master.            */
                                
#ifndef CMD_SETPGADDR               /* If this is disabled, applications can only be flashed starting      */
#define CMD_SETPGADDR   false       /* from page 0, this is OK for most applications.                      */
#endif /* CMD_SETPGADDR */          /* If this is enabled, Timonel expects STPGADDR before each data page. */
                                    /* Enabling this option is MANDATORY when AUTO_PAGE_ADDR is disabled.  */
                                
#ifndef TWO_STEP_INIT               /* If this is enabled, Timonel expects a two-step initialization from  */
#define TWO_STEP_INIT   false       /* an I2C master for starting. Otherwise, single-step init is enough   */
#endif /* TWO_STEP_INIT */

#ifndef USE_WDT_RESET               /* Use watchdog for resetting instead of jumping to TIMONEL_START.     */
#define USE_WDT_RESET   true    
#endif /* USE_WDT_RESET */

#ifndef CHECK_EMPTY_FL              /* GETTMNLV will read the first 100 flash memory positions to check if */
#define CHECK_EMPTY_FL  false       /* there is an application (or some other data) loaded.                */
#endif /* CHECK_EMPTY_FL */

#ifndef CMD_READFLASH               /* This option enables the READFLSH command. It can be useful for      */
#define CMD_READFLASH   false       /* backing up the flash memory before flashing a new firmware.         */
#endif /* CMD_READFLASH */                                   

/* ^^^^^^ [       End of feature settings shown in the GETTMNLV command.       ] ^^^^^^ */
/* ====== [       ......................................................       ] ====== */

/* ------------------------------------------------------------------------------------ */
/* ---                    Timonel general features and settings                     --- */
/* ------------------------------------------------------------------------------------ */

// Timonel optional features
#ifndef AUTO_CLK_TWEAK              /* When this feature is enabled, the clock speed adjustment is made at */
#define AUTO_CLK_TWEAK  false       /* run time based on the low fuse setup. It works only for internal    */
#endif /* AUTO_CLK_TWEAK */         /* CPU clock configurations: RC oscillator or HF PLL.                  */

#ifndef LOW_FUSE                    /* If AUTO_CLK_TWEAK is disabled (default), this value defines all the */
#define LOW_FUSE        0x62        /* speed adjustments that are fixed at compile time. It has to match   */
#endif /* LOW_FUSE */               /* the actual low fuse setup, otherwise Timonel probably won't work.   */

#ifndef LED_UI_PIN
#define LED_UI_PIN      PB1         /* GPIO pin to monitor activity. If ENABLE_LED_UI is enabled, some     */
#endif /* LED_UI_PIN */             /* bootloader commands could activate it at run time. Please check the */
                                    /* connections to be sure that there are no power circuits attached.   */

/* ------------------------------------------------------------------------------------ */
/* ---    Timonel internal configuration. Do not change anything below this line    --- */
/* ---    unless you know how to customize or adapt it to another microcontroller.  --- */
/* ---    Below options cannot be modified in "tml-config.mak", only in this file.  --- */
/* ------------------------------------------------------------------------------------ */

// TWI commands Xmit packet size
#define MST_PACKET_SIZE 32          /* Master-to-slave Xmit packet size: always even values, min=2, max=32 */
#define SLV_PACKET_SIZE 32          /* Slave-to-master Xmit packet size: always even values, min=2, max=32 */

// Led UI settings                           
#define LED_UI_DDR      DDRB        /* Activity monitor led data register.                                 */
#define LED_UI_PORT     PORTB       /* Activity monitor led port.                                          */

// Timonel ID characters
#if !(AUTO_CLK_TWEAK)               /* If clock tweaking is made at compile time, use uppercase ID chars.  */
#define ID_CHAR_1       78          /* N */
#define ID_CHAR_2       66          /* B */
#define ID_CHAR_3       84          /* T */
#else                               /* If automatic clock tweaking is made at run time, use lowercase ID.  */
#define ID_CHAR_1       110         /* n */
#define ID_CHAR_2       98          /* b */
#define ID_CHAR_3       116         /* t */
#endif /* !(AUTO_CLK_TWEAK) */

// Flags byte
#define FL_INIT_1       0           /* Flag bit 1 (1)  : Two-step initialization STEP 1 */
#define FL_INIT_2       1           /* Flag bit 2 (2)  : Two-step initialization STEP 2 */
#define FL_DEL_FLASH    2           /* Flag bit 3 (4)  : Delete flash memory            */
#define FL_EXIT_TML     3           /* Flag bit 4 (8)  : Exit Timonel & run application */
#define FL_BIT_5        4           /* Flag bit 5 (16) : Not used */
#define FL_BIT_6        5           /* Flag bit 6 (32) : Not used */
#define FL_BIT_7        6           /* Flag bit 7 (64) : Not used */
#define FL_BIT_8        7           /* Flag bit 8 (128): Not used */

// Command reply length constants
#define GETTMNLV_RPLYLN 11          /* GETTMNLV command reply length */
#define STPGADDR_RPLYLN 2           /* STPGADDR command reply length */
#define WRITPAGE_RPLYLN 2           /* WRITPAGE command reply length */

// Memory page definitions
#define RESET_PAGE      0           /* Interrupt vector table address start location. */

// Fuses' constants
#define L_FUSE_ADDR     0x0000      /* Low fuse register address */
#define H_FUSE_ADDR     0x0003      /* High fuse register address */
#define E_FUSE_ADDR     0x0002      /* Extended fuse register address */
#define HFPLL_CLK_SRC   0x01        /* HF PLL (16 MHz) clock source low fuse value */
#define RCOSC_CLK_SRC   0x02        /* RC oscillator (8 MHz) clock source low fuse value */
#define LFUSE_PRESC_BIT 7           /* Prescaler bit position in low fuse (FUSE_CKDIV8) */

// Non-blocking delays
#define SHORT_EXIT_DLY  0x0A        /* Long exit delay */
#define LONG_EXIT_DLY   0x30        /* Short exit delay */
#define SHORT_LED_DLY   0xFF        /* Long led delay */
#define LONG_LED_DLY    0x1FF       /* Short led delay */

// CPU clock calibration value
#define OSC_FAST        0x4C        /* Offset for when the low fuse is set below 16 MHz. */

// Extra safety measures
#define FORCE_ERASE_PG  false       /* If this option is enabled, each flash memory page is erased before  */
                                    /* writing new data. Normally, it shouldn't be necessary to enable it. */

#define CLEAR_BIT_7_R31 false       /* This is to avoid that the first bootloader instruction is skipped   */
                                    /* after restarting without an user application in memory. See:        */
                                    /* http://www.avrfreaks.net/comment/2561866#comment-2561866            */
                                    
#define CHECK_PAGE_IX   false       /* If this option is enabled, the page index size is checked to ensure */
                                    /* that isn't bigger than SPM_PAGESIZE (usually 64 bytes). This keeps  */
                                    /* the app data integrity in case the master sends wrong page sizes.   */
									
// **************************************************************
// * INVESTIGATE THIS CODE *************************************			
// **************************************************************									
#define DELETE_OVF_APP  true		/* If this is enabled, the application being uploaded will be deleted  */
									/* immediately if the TWI master tries to write data beyond the        */
									/* trampoline bytes (or page, depending on setup) to protect Timonel.  */
// **************************************************************
// * END OF CODE TO INVESTIGATE ********************************			
// **************************************************************

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
#endif /* ENABLE_LED_UI */
#if (AUTO_PAGE_ADDR == true)
    #define FT_BIT_1    2
#else
    #define FT_BIT_1    0
#endif /* AUTO_PAGE_ADDR */
#if (APP_USE_TPL_PG == true)
    #define FT_BIT_2    4
#else
    #define FT_BIT_2    0
#endif /* APP_USE_TPL_PG */
#if (CMD_SETPGADDR == true)
    #define FT_BIT_3    8
#else
    #define FT_BIT_3    0
#endif /* CMD_SETPGADDR */
#if (TWO_STEP_INIT == true)
    #define FT_BIT_4    16
#else
    #define FT_BIT_4    0
#endif /* TWO_STEP_INIT */
#if (USE_WDT_RESET == true)
    #define FT_BIT_5    32
#else
    #define FT_BIT_5    0
#endif /* USE_WDT_RESET */
#if (CHECK_EMPTY_FL == true)
    #define FT_BIT_6    64
#else
    #define FT_BIT_6    0
#endif /* CHECK_EMPTY_FL */
#if (CMD_READFLASH == true)
    #define FT_BIT_7    128
#else
    #define FT_BIT_7    0
#endif /* CMD_READFLASH */

#define TML_FEATURES (FT_BIT_7 + FT_BIT_6 + FT_BIT_5 + FT_BIT_4 + FT_BIT_3 + FT_BIT_2 + FT_BIT_1 + FT_BIT_0)

/////////////////////////////////////////////////////////////////////////////
////////////      ALL USI TWI DRIVER CONFIG BELOW THIS LINE      ////////////
/////////////////////////////////////////////////////////////////////////////

// Driver buffer definitions
// Allowed RX buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128 or 256
#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE  64
#endif /* TWI_RX_BUFFER_SIZE */

#define TWI_RX_BUFFER_MASK (TWI_RX_BUFFER_SIZE - 1)

#if (TWI_RX_BUFFER_SIZE & TWI_RX_BUFFER_MASK)
#error TWI RX buffer size is not a power of 2
#endif /* TWI_RX_BUFFER_SIZE & TWI_RX_BUFFER_MASK */

// Allowed TX buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128 or 256
#ifndef TWI_TX_BUFFER_SIZE
#define TWI_TX_BUFFER_SIZE  64
#endif /* TWI_TX_BUFFER_SIZE */

#define TWI_TX_BUFFER_MASK (TWI_TX_BUFFER_SIZE - 1)

#if (TWI_TX_BUFFER_SIZE & TWI_TX_BUFFER_MASK)
#error TWI TX buffer size is not a power of 2
#endif /* TWI_TX_BUFFER_SIZE & TWI_TX_BUFFER_MASK */

// Device dependent defines
#if defined(__AVR_ATtiny25__) | \
    defined(__AVR_ATtiny45__) | \
    defined(__AVR_ATtiny85__)
#define DDR_USI DDRB
#define PORT_USI PORTB
#define PIN_USI PINB
#define PORT_USI_SDA PB0
#define PORT_USI_SCL PB2
#define PIN_USI_SDA PINB0
#define PIN_USI_SCL PINB2
#define TWI_START_COND_FLAG USISIF  /* This status register flag indicates that an I2C START condition occurred on the bus (can trigger an interrupt) */
#define USI_OVERFLOW_FLAG USIOIF    /* This status register flag indicates that the bits reception or transmission is complete (can trigger an interrupt) */
#define TWI_STOP_COND_FLAG USIPF    /* This status register flag indicates that an I2C STOP condition occurred on the bus */
#define TWI_COLLISION_FLAG USIDC    /* This status register flag indicates that a data output collision occurred on the bus */
#define TWI_START_COND_INT USISIE   /* This control register bit defines whether an I2C START condition will trigger an interrupt */
#define USI_OVERFLOW_INT USIOIE     /* This control register bit defines whether an USI 4-bit counter overflow will trigger an interrupt */
#endif /* ATtinyX5 */

#endif /* _TML_CONFIG_H_ */
