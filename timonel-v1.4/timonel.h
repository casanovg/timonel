/*
 *  Timonel - TWI Bootloader for TinyX5 MCUs
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: timonel.h (Main bootloader headers)
 *  ........................................... 
 *  Version: 1.4 "Sandra" / 2019-08-09 (BENI)
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
/*            NOTE: These values can be set externally as makefile options              */

// Features Byte
// =============

// Bit 1
#ifndef ENABLE_LED_UI               /* If this is enabled, LED_UI_PIN is used to display Timonel activity. */
#define ENABLE_LED_UI   false       /* PLEASE DISABLE THIS FOR PRODUCTION! IT COULD ACTIVATE SOMETHING     */
#endif /* ENABLE_LED_UI */          /* CONNECTED TO A POWER SOURCE BY ACCIDENT!                            */

// Bit 2           
#ifndef AUTO_PAGE_ADDR              /* Automatic page and trampoline address calculation. If this option   */
#define AUTO_PAGE_ADDR  true        /* is disabled, the trampoline has to be calculated and written by the */
#endif /* AUTO_PAGE_ADDR */         /* I2C master. Therefore, enabling CMD_SETPGADDR becomes mandatory.    */

// Bit 3                              
#ifndef APP_USE_TPL_PG              /* Allow the user appl. to use the trampoline page when AUTO_PAGE_ADDR */
#define APP_USE_TPL_PG  false       /* is enabled. This is a safety measure since enabling this takes 2    */
#endif /* APP_USE_TPL_PG */         /* extra memory pages. In the end, disabling this allows 1 extra page. */
                                    /* If AUTO_PAGE_ADDR is disabled, this option is irrelevant since the  */
                                    /* trampoline page writing is to be made by the I2C master.            */

// Bit 4                                
#ifndef CMD_SETPGADDR               /* If this is disabled, applications can only be flashed starting      */
#define CMD_SETPGADDR   false       /* from page 0, this is OK for most applications.                      */
#endif /* CMD_SETPGADDR */          /* If this is enabled, Timonel expects STPGADDR before each data page. */
                                    /* Enabling this option is MANDATORY when AUTO_PAGE_ADDR is disabled.  */

// Bit 5                              
#ifndef TWO_STEP_INIT               /* If this is enabled, Timonel expects a two-step initialization from  */
#define TWO_STEP_INIT   false       /* an I2C master for starting. Otherwise, single-step init is enough   */
#endif /* TWO_STEP_INIT */

// Bit 6
#ifndef USE_WDT_RESET               /* Use watchdog for resetting instead of jumping to TIMONEL_START.     */
#define USE_WDT_RESET   true    
#endif /* USE_WDT_RESET */

// Bit 7
#ifndef TIMEOUT_EXIT                /* If this option is set to true (1), the user application loaded will */
#define TIMEOUT_EXIT    false       /* NOT start automatically after a timeout when the bootloader is not  */
#endif /* TIMEOUT_EXIT */           /* initialized. It always has to be launched by the TWI master.        */

// Bit 8
#ifndef CMD_READFLASH               /* This option enables the READFLSH command. It can be useful for      */
#define CMD_READFLASH   false       /* backing up the flash memory before flashing a new firmware.         */
#endif /* CMD_READFLASH */                                   

// Extended Features Byte
// ======================

// Bit 1
#ifndef AUTO_CLK_TWEAK              /* When this feature is enabled, the clock speed adjustment is made at */
#define AUTO_CLK_TWEAK  false       /* run time by reading low fuse value. It works only for internal CPU  */
#endif /* AUTO_CLK_TWEAK */         /* clock configurations: RC oscillator or HF PLL.                      */
                                    /* NOTE: This value can be set externally as a makefile option         */

// Bit 2
#define FORCE_ERASE_PG  false       /* If this option is enabled, each flash memory page is erased before  */
                                    /* writing new data. Normally, it shouldn't be necessary to enable it. */

// Bit 3
#define CLEAR_BIT_7_R31 false       /* This is to avoid that the first bootloader instruction is skipped   */
                                    /* after restarting without an user application in memory. See:        */
                                    /* http://www.avrfreaks.net/comment/2561866#comment-2561866            */
                                    
// Bit 4
#define CHECK_PAGE_IX   false       /* If this option is enabled, the page index size is checked to ensure */
                                    /* that isn't bigger than SPM_PAGESIZE (usually 64 bytes). This keeps  */
                                    /* the app data integrity in case the master sends wrong page sizes.   */

/* ^^^^^^ [       End of feature settings shown in the GETTMNLV command.       ] ^^^^^^ */
/* ====== [       ......................................................       ] ====== */

/* ------------------------------------------------------------------------------------ */
/* ---    Timonel internal configuration. Do not change anything below this line    --- */
/* ---    unless you know how to customize or adapt it to another microcontroller.  --- */
/* ---    Below options cannot be modified in "tml-config.mak", only in this file.  --- */
/* ------------------------------------------------------------------------------------ */
                                    
// TWI commands Xmit packet size
#define MST_PACKET_SIZE 32          /* Master-to-slave Xmit packet size: always even values, min=2, max=32 */
#define SLV_PACKET_SIZE 32          /* Slave-to-master Xmit packet size: always even values, min=2, max=32 */

// Led UI settings
#ifndef LED_UI_PIN                  /* GPIO pin to monitor activity. If ENABLE_LED_UI is enabled, some     */
#define LED_UI_PIN      PB1         /* bootloader commands could activate it at run time. Please check the */
#endif /* LED_UI_PIN */             /* connections to be sure that there are no power circuits attached.   */
                                    /* NOTE: This value can be set externally as a makefile option and it  */
                                    /* is shown in the GETTMNLV command.                                   */
#define LED_UI_DDR      DDRB        /* Activity monitor led data register.                                 */
#define LED_UI_PORT     PORTB       /* Activity monitor led port.                                          */

// Timonel ID characters
#define ID_CHAR_1       78          /* N */
#define ID_CHAR_2       66          /* B */
#define ID_CHAR_3       84          /* T */

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
#define GETTMNLV_RPLYLN 12          /* GETTMNLV command reply length */
#define STPGADDR_RPLYLN 2           /* STPGADDR command reply length */
#define WRITPAGE_RPLYLN 2           /* WRITPAGE command reply length */

// Memory page definitions
#define RESET_PAGE      0           /* Interrupt vector table address start location. */

// Fuses' constants
#ifndef LOW_FUSE                    /* When AUTO_CLK_TWEAK is disabled, this value must match the low fuse */
#define LOW_FUSE        0x62        /* setting, otherwise, the bootloader will not work. If AUTO_CLK_TWEAK */
#endif /* LOW_FUSE */               /* is enabled, this value is irrelevant.                               */
                                    /* NOTE: This value can be set externally as a makefile option and it  */
                                    /* is shown in the GETTMNLV command.                                   */
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
#define OSC_FAST        0x4C        /* Offset for when the low fuse is set below 16 MHz.   */
                                    /* NOTE: The sum of this value plus the factory OSCCAL */
                                    /* value is shown in the GETTMNLV command.             */

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

// Extended features code calculation for GETTMNLV replies
#if (AUTO_CLK_TWEAK == true)
    #define EF_BIT_0    1
#else
    #define EF_BIT_0    0
#endif /* AUTO_CLK_TWEAK */
#if (FORCE_ERASE_PG == true)
    #define EF_BIT_1    2
#else
    #define EF_BIT_1    0
#endif /* FORCE_ERASE_PG */
#if (CLEAR_BIT_7_R31 == true)
    #define EF_BIT_2    4
#else
    #define EF_BIT_2    0
#endif /* CLEAR_BIT_7_R31 */
#if (CHECK_PAGE_IX == true)
    #define EF_BIT_3    8
#else
    #define EF_BIT_3    0
#endif /* CHECK_PAGE_IX */
#define EF_BIT_4    0       /* EF Bit 4 not used */
#define EF_BIT_5    0       /* EF Bit 5 not used */
#define EF_BIT_6    0       /* EF Bit 6 not used */
#define EF_BIT_7    0       /* EF Bit 7 not used */

#define TML_EXT_FEATURES (EF_BIT_7 + EF_BIT_6 + EF_BIT_5 + EF_BIT_4 + EF_BIT_3 + EF_BIT_2 + EF_BIT_1 + EF_BIT_0)

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
