/*
 *  Timonel - TWI Bootloader for ATtiny MCUs
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: timonel.h (Main bootloader headers)
 *  ........................................... 
 *  Version: 1.6 "Sandra" / 2020-10-29
 *  gustavo.casanova@nicebots.com
 *  ...........................................
 */

/*
 *****************************************************************************
 * Please do NOT modify this file directly, change "tml-config.mak" instead! *
 *****************************************************************************
 */

#ifndef TML_CONFIG_H
#define TML_CONFIG_H

//#pragma message "   >>>   Run, Timonel, run!   <<<   "

// Includes
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../../nb-twi-cmd/src/nb-twi-cmd.h"

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

/* ====== [   The configuration of the next optional features can be checked   ] ====== */
/* VVVVVV [   from the I2C master by using the GETTMNLV command.               ] VVVVVV */
/*            NOTE: These values can be set externally as makefile options              */

// Features Byte
// =============

// Bit 0
#ifndef ENABLE_LED_UI       /* If this is enabled, LED_UI_PIN is used to display Timonel activity. */
#define ENABLE_LED_UI false /* PLEASE DISABLE THIS FOR PRODUCTION! IT COULD ACTIVATE SOMETHING     */
#endif /* ENABLE_LED_UI */  /* CONNECTED TO A POWER SOURCE BY ACCIDENT!                            */

// Bit 1
#ifndef AUTO_PAGE_ADDR      /* Automatic page and trampoline address calculation. If this option   */
#define AUTO_PAGE_ADDR true /* is disabled, the trampoline has to be calculated and written by the */
#endif /* AUTO_PAGE_ADDR */ /* I2C master. Therefore, enabling CMD_SETPGADDR becomes mandatory.    */

// Bit 2
#ifndef APP_USE_TPL_PG       /* Allow the user appl. to use the trampoline page when AUTO_PAGE_ADDR */
#define APP_USE_TPL_PG false /* is enabled. This is a safety measure since enabling this takes 2    */
#endif /* APP_USE_TPL_PG */  /* extra memory pages. In the end, disabling this allows 1 extra page. */
                             /* If AUTO_PAGE_ADDR is disabled, this option is irrelevant since the  */
                             /* trampoline page writing is to be made by the I2C master.            */

// Bit 3
#ifndef CMD_SETPGADDR       /* If this is disabled, applications can only be flashed starting      */
#define CMD_SETPGADDR false /* from page 0, this is OK for most applications.                      */
#endif /* CMD_SETPGADDR */  /* If this is enabled, Timonel expects STPGADDR before each data page. */
                            /* Enabling this option is MANDATORY when AUTO_PAGE_ADDR is disabled.  */

// Bit 4
#ifndef TWO_STEP_INIT       /* If this is enabled, Timonel expects a two-step initialization from  */
#define TWO_STEP_INIT false /* an I2C master for starting. Otherwise, single-step init is enough.  */
#endif                      /* TWO_STEP_INIT */

// Bit 5
#ifndef USE_WDT_RESET /* Use watchdog for resetting instead of jumping to TIMONEL_START.     */
#define USE_WDT_RESET true
#endif /* USE_WDT_RESET */

// Bit 6
#ifndef APP_AUTORUN      /* If this option is set to true (1), the user application loaded will */
#define APP_AUTORUN true /* start automatically after a timeout when the bootloader is not      */
#endif /* APP_AUTORUN */ /* initialized. Otherwise, It has to be launched by the TWI master.    */

// Bit 7
#ifndef CMD_READFLASH       /* This option enables the READFLSH command. It can be useful for      */
#define CMD_READFLASH false /* backing up the flash memory before flashing a new firmware.         */
#endif                      /* CMD_READFLASH */

// Extended Features Byte
// ======================

// Bit 0
#ifndef AUTO_CLK_TWEAK       /* When this feature is enabled, the clock speed adjustment is made at */
#define AUTO_CLK_TWEAK false /* run time by reading low fuse value. It works only for internal CPU  */
#endif /* AUTO_CLK_TWEAK */  /* clock configurations: RC oscillator or HF PLL.                      */
                             /* NOTE: This value can be set externally as a makefile option         */

// Bit 1
#define FORCE_ERASE_PG false /* If this option is enabled, each flash memory page is erased before  */
                             /* writing new data. Normally, it shouldn't be necessary to enable it. */

// Bit 2
#define CLEAR_BIT_7_R31 false /* This is to avoid that the first bootloader instruction is skipped   */
                              /* after restarting without an user application in memory. See:        */
                              /* http://www.avrfreaks.net/comment/2561866#comment-2561866            */

// Bit 3
#define CHECK_PAGE_IX false /* If this option is enabled, the page index size is checked to ensure */
                            /* that isn't bigger than SPM_PAGESIZE (usually 64 bytes). This keeps  */
                            /* the app data integrity in case the master sends wrong page sizes.   */

// Bit 4
#ifndef CMD_READDEVS
#define CMD_READDEVS false /* This option enables the READDEVS command. It allows reading all     */
#endif /* CMD_READDEVS */  /* fuse bits, lock bits, and device signature imprint table.           */

// Bit 5
#ifndef EEPROM_ACCESS       /* This option enables the READEEPR and WRITEEPR commands, which allow */
#define EEPROM_ACCESS false /* reading and writing the device EEPROM.                              */
#endif                      /* EEPROM_ACCESS */

/* ^^^^^^ [       End of feature settings shown in the GETTMNLV command.       ] ^^^^^^ */
/* ====== [       ......................................................       ] ====== */

/* ------------------------------------------------------------------------------------ */
/* ---    Timonel internal configuration. Do not change anything below this line    --- */
/* ---    unless you know how to customize or adapt it to another microcontroller.  --- */
/* ---    Below options cannot be modified in "tml-config.mak", only in this file.  --- */
/* ------------------------------------------------------------------------------------ */

// TWI commands Xmit packet size
#define MST_PACKET_SIZE 32 /* Master-to-slave Xmit packet size: always even values, min=2, max=32 */
#define SLV_PACKET_SIZE 32 /* Slave-to-master Xmit packet size: always even values, min=2, max=32 */

// Led UI settings
#ifndef LED_UI_PIN        /* GPIO pin to monitor activity. If ENABLE_LED_UI is enabled, some     */
#define LED_UI_PIN PB1    /* bootloader commands could activate it at run time. Please check the */
#endif /* LED_UI_PIN */   /* connections to be sure that there are no power circuits attached.   */
                          /* NOTE: This value can be set externally as a makefile option and it  */
                          /* is shown in the GETTMNLV command.                                   */
#define LED_UI_DDR DDRB   /* Activity monitor led data register.                                 */
#define LED_UI_PORT PORTB /* Activity monitor led port.                                          */

// Timonel ID characters
#define ID_CHAR_1 78  /* N */
#define ID_CHAR_2 66  /* B */
#define ID_CHAR_3 84  /* T */
#define ID_CHAR_4 116 /* t */

// Flags byte
#define FL_INIT_1 0    /* Flag bit 1 (1)  : Two-step initialization STEP 1 */
#define FL_INIT_2 1    /* Flag bit 2 (2)  : Two-step initialization STEP 2 */
#define FL_DEL_FLASH 2 /* Flag bit 3 (4)  : Delete flash memory            */
#define FL_EXIT_TML 3  /* Flag bit 4 (8)  : Exit Timonel & run application */
#define FL_BIT_5 4     /* Flag bit 5 (16) : Not used */
#define FL_BIT_6 5     /* Flag bit 6 (32) : Not used */
#define FL_BIT_7 6     /* Flag bit 7 (64) : Not used */
#define FL_BIT_8 7     /* Flag bit 8 (128): Not used */

// Length constants for command replies
#define GETTMNLV_RPLYLN 12 /* GETTMNLV command reply length */
#define STPGADDR_RPLYLN 2  /* STPGADDR command reply length */
#define WRITPAGE_RPLYLN 2  /* WRITPAGE command reply length */
#define READDEVS_RPLYLN 10 /* READDEVS command reply length */
#define WRITEEPR_RPLYLN 2  /* WRITEEPR command reply length */
#define READEEPR_RPLYLN 3  /* READEEPR command reply length */

// Memory page definitions
#define RESET_PAGE 0 /* Interrupt vector table address start location. */

// Fuses' constants
#ifndef LOW_FUSE           /* When AUTO_CLK_TWEAK is disabled, this value must match the low fuse */
#define LOW_FUSE 0x62      /* setting, otherwise, the bootloader will not work. If AUTO_CLK_TWEAK */
#endif /* LOW_FUSE */      /* is enabled, this value is irrelevant.                               */
                           /* NOTE: This value can be set externally as a makefile option and it  */
                           /* is shown in the GETTMNLV command.                                   */
#define HFPLL_CLK_SRC 0x01 /* HF PLL (16 MHz) clock source low fuse value */
#define RCOSC_CLK_SRC 0x02 /* RC oscillator (8 MHz) clock source low fuse value */
#define LFUSE_PRESC_BIT 7  /* Prescaler bit position in low fuse (FUSE_CKDIV8) */

// Non-blocking delays
#define SHORT_EXIT_DLY 0x0A /* Long exit delay */
#define LONG_EXIT_DLY 0x30  /* Short exit delay */
#define SHORT_LED_DLY 0xFF  /* Long led delay */
#define LONG_LED_DLY 0x1FF  /* Short led delay */

// CPU clock calibration value
#define OSC_FAST 0x4C /* Offset for when the low fuse is set below 16 MHz.   */
                      /* NOTE: The sum of this value plus the factory OSCCAL */
                      /* value is shown in the GETTMNLV command.             */

// Erase temporary page buffer macro
#define BOOT_TEMP_BUFF_ERASE (_BV(__SPM_ENABLE) | _BV(CTPB))
#define boot_temp_buff_erase()                       \
    (__extension__({                                 \
        __asm__ __volatile__(                        \
            "sts %0, %1\n\t"                         \
            "spm\n\t"                                \
            :                                        \
            : "i"(_SFR_MEM_ADDR(__SPM_REG)),         \
              "r"((uint8_t)(BOOT_TEMP_BUFF_ERASE))); \
    }))

// "Boot.h" patch to read the signature bytes. For some reason, the SIGRD
// flag definition is missing in some header files, including the ATtiny85.
#ifndef SIGRD
#define SIGRD RSIG
#endif /* SIGRD */

// Features code calculation for GETTMNLV replies
#if (ENABLE_LED_UI == true)
#define FT_BIT_0 1
#else
#define FT_BIT_0 0
#endif /* ENABLE_LED_UI */
#if (AUTO_PAGE_ADDR == true)
#define FT_BIT_1 2
#else
#define FT_BIT_1 0
#endif /* AUTO_PAGE_ADDR */
#if (APP_USE_TPL_PG == true)
#define FT_BIT_2 4
#else
#define FT_BIT_2 0
#endif /* APP_USE_TPL_PG */
#if (CMD_SETPGADDR == true)
#define FT_BIT_3 8
#else
#define FT_BIT_3 0
#endif /* CMD_SETPGADDR */
#if (TWO_STEP_INIT == true)
#define FT_BIT_4 16
#else
#define FT_BIT_4 0
#endif /* TWO_STEP_INIT */
#if (USE_WDT_RESET == true)
#define FT_BIT_5 32
#else
#define FT_BIT_5 0
#endif /* USE_WDT_RESET */
#if (APP_AUTORUN == true)
#define FT_BIT_6 64
#else
#define FT_BIT_6 0
#endif /* APP_AUTORUN */
#if (CMD_READFLASH == true)
#define FT_BIT_7 128
#else
#define FT_BIT_7 0
#endif /* CMD_READFLASH */

#define TML_FEATURES (FT_BIT_7 + FT_BIT_6 + FT_BIT_5 + FT_BIT_4 + FT_BIT_3 + FT_BIT_2 + FT_BIT_1 + FT_BIT_0)

// Extended features code calculation for GETTMNLV replies
#if (AUTO_CLK_TWEAK == true)
#define EF_BIT_0 1
#else
#define EF_BIT_0 0
#endif /* AUTO_CLK_TWEAK */
#if (FORCE_ERASE_PG == true)
#define EF_BIT_1 2
#else
#define EF_BIT_1 0
#endif /* FORCE_ERASE_PG */
#if (CLEAR_BIT_7_R31 == true)
#define EF_BIT_2 4
#else
#define EF_BIT_2 0
#endif /* CLEAR_BIT_7_R31 */
#if (CHECK_PAGE_IX == true)
#define EF_BIT_3 8
#else
#define EF_BIT_3 0
#endif /* CHECK_PAGE_IX */
#if (CMD_READDEVS == true)
#define EF_BIT_4 16
#else
#define EF_BIT_4 0
#endif /* CMD_READDEVS */
#if (EEPROM_ACCESS == true)
#define EF_BIT_5 32
#else
#define EF_BIT_5 0
#endif             /* EEPROM_ACCESS */
#define EF_BIT_6 0 /* EF Bit 6 not used */
#define EF_BIT_7 0 /* EF Bit 7 not used */

#define TML_EXT_FEATURES (EF_BIT_7 + EF_BIT_6 + EF_BIT_5 + EF_BIT_4 + EF_BIT_3 + EF_BIT_2 + EF_BIT_1 + EF_BIT_0)

/////////////////////////////////////////////////////////////////////////////
////////////      ALL USI TWI DRIVER CONFIG BELOW THIS LINE      ////////////
/////////////////////////////////////////////////////////////////////////////

// Driver buffer definitions
// Allowed RX buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128 or 256
#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE 64
#endif /* TWI_RX_BUFFER_SIZE */

#define TWI_RX_BUFFER_MASK (TWI_RX_BUFFER_SIZE - 1)

#if (TWI_RX_BUFFER_SIZE & TWI_RX_BUFFER_MASK)
#error TWI RX buffer size is not a power of 2
#endif /* TWI_RX_BUFFER_SIZE & TWI_RX_BUFFER_MASK */

// Allowed TX buffer sizes: 1, 2, 4, 8, 16, 32, 64, 128 or 256
#ifndef TWI_TX_BUFFER_SIZE
#define TWI_TX_BUFFER_SIZE 64
#endif /* TWI_TX_BUFFER_SIZE */

#define TWI_TX_BUFFER_MASK (TWI_TX_BUFFER_SIZE - 1)

#if (TWI_TX_BUFFER_SIZE & TWI_TX_BUFFER_MASK)
#error TWI TX buffer size is not a power of 2
#endif /* TWI_TX_BUFFER_SIZE & TWI_TX_BUFFER_MASK */

// Pointer-to-function type
typedef void (*const fptr_t)(void);

// USI TWI driver operational modes
typedef enum {
    STATE_CHECK_RECEIVED_ADDRESS = 0,
    STATE_SEND_DATA_BYTE = 1,
    STATE_RECEIVE_ACK_AFTER_SENDING_DATA = 2,
    STATE_CHECK_RECEIVED_ACK = 3,
    STATE_RECEIVE_DATA_BYTE = 4,
    STATE_PUT_BYTE_IN_RX_BUFFER_AND_SEND_ACK = 5
} OverflowState;

// USI TWI driver globals
static uint8_t rx_buffer[TWI_RX_BUFFER_SIZE];
static uint8_t tx_buffer[TWI_TX_BUFFER_SIZE];
static uint8_t rx_byte_count = 0;  // Bytes received in RX buffer
static uint8_t rx_head = 0, rx_tail = 0;
static uint8_t tx_head = 0, tx_tail = 0;
static OverflowState device_state;

// USI TWI hardware mapping
// ------------------------
// DDR_USI = I2C data direction register
// PORT_USI = I2C output register
// PIN_USI = I2C input register
// PORT_USI_SDA = I2C SDA output register
// PORT_USI_SCL = I2C SCL output register
// PIN_USI_SDA = I2C SDA input register
// PIN_USI_SCL = I2C SDL input register
// TWI_START_COND_FLAG = Status register flag: indicates an I2C START condition on the bus (can trigger an interrupt)
// USI_OVERFLOW_FLAG = Status register flag: indicates a complete bit reception/transmission (can trigger an interrupt)
// TWI_STOP_COND_FLAG = Status register flag: indicates an I2C STOP condition on the bus
// TWI_COLLISION_FLAG = Status register flag: indicates a data output collision on the bus
// TWI_START_COND_INT = Control register bit: defines whether an I2C START condition triggers an interrupt
// USI_OVERFLOW_INT = Control register bit: defines whether a USI 4-bit counter overflow triggers an interrupt

// ATtinyX5
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
#define TWI_START_COND_FLAG USISIF
#define USI_OVERFLOW_FLAG USIOIF
#define TWI_STOP_COND_FLAG USIPF
#define TWI_COLLISION_FLAG USIDC
#define TWI_START_COND_INT USISIE
#define USI_OVERFLOW_INT USIOIE
#endif // ATtinyX5

// ATtinyX4
#if defined(__AVR_ATtiny24__) | \
    defined(__AVR_ATtiny44__) | \
    defined(__AVR_ATtiny84__)
#define DDR_USI DDRA
#define PORT_USI PORTA
#define PIN_USI PINA
#define PORT_USI_SDA PORTA6
#define PORT_USI_SCL PORTA4
#define PIN_USI_SDA PINA6
#define PIN_USI_SCL PINA4
#define TWI_START_COND_FLAG USISIF
#define USI_OVERFLOW_FLAG USIOIF
#define TWI_STOP_COND_FLAG USIPF
#define TWI_COLLISION_FLAG USIDC
#define TWI_START_COND_INT USISIE
#define USI_OVERFLOW_INT USIOIE
#endif

// ATtinyX313
#if defined(__AVR_ATtiny2313__) | \
    defined(__AVR_ATtiny4313__)
#define DDR_USI DDRB
#define PORT_USI PORTB
#define PIN_USI PINB
#define PORT_USI_SDA PB5
#define PORT_USI_SCL PB7
#define PIN_USI_SDA PINB5
#define PIN_USI_SCL PINB7
#define TWI_START_COND_FLAG USISIF
#define USI_OVERFLOW_FLAG USIOIF
#define TWI_STOP_COND_FLAG USIPF
#define TWI_COLLISION_FLAG USIDC
#define TWI_START_COND_INT USISIE
#define USI_OVERFLOW_INT USIOIE
#endif

// ATtinyX7
#if defined(__AVR_ATtiny87__) | \
    defined(__AVR_ATtiny167__)
#define DDR_USI DDRB
#define PORT_USI PORTB
#define PIN_USI PINB
#define PORT_USI_SDA PB0
#define PORT_USI_SCL PB2
#define PIN_USI_SDA PINB0
#define PIN_USI_SCL PINB2
#define TWI_START_COND_FLAG USISIF
#define USI_OVERFLOW_FLAG USIOIF
#define TWI_STOP_COND_FLAG USIPF
#define TWI_COLLISION_FLAG USIDC
#define TWI_START_COND_INT USISIE
#define USI_OVERFLOW_INT USIOIE
#endif

// ATtinyX61
#if defined(__AVR_ATtiny261__) | \
    defined(__AVR_ATtiny461__) | \
    defined(__AVR_ATtiny861__)
#define DDR_USI DDRB
#define PORT_USI PORTB
#define PIN_USI PINB
#define PORT_USI_SDA PB0
#define PORT_USI_SCL PB2
#define PIN_USI_SDA PINB0
#define PIN_USI_SCL PINB2
#define TWI_START_COND_FLAG USISIF
#define USI_OVERFLOW_FLAG USIOIF
#define TWI_STOP_COND_FLAG USIPF
#define TWI_COLLISION_FLAG USIDC
#define TWI_START_COND_INT USISIE
#define USI_OVERFLOW_INT USIOIE
#endif

// ATtiny43
#if defined(__AVR_ATtiny43U__)
#define DDR_USI DDRB
#define PORT_USI PORTB
#define PIN_USI PINB
#define PORT_USI_SDA PORTB4
#define PORT_USI_SCL PORTB6
#define PIN_USI_SDA PINB4
#define PIN_USI_SCL PINB6
#define TWI_START_COND_FLAG USISIF
#define USI_OVERFLOW_FLAG USIOIF
#define TWI_STOP_COND_FLAG USIPF
#define TWI_COLLISION_FLAG USIDC
#define TWI_START_COND_INT USISIE
#define USI_OVERFLOW_INT USIOIE
#endif

#endif  // TML_CONFIG_H
