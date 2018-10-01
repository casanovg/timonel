/*
 *  File: tml-config.h
 *  Project: Timonel - I2C Bootloader for ATtiny85 MCUs
 *  Author: Gustavo Casanova
 *  .......................................................
 *  2018-09-16 gustavo.casanova@nicebots.com
 */

#ifndef _TML_CONFIG_H_
#define _TML_CONFIG_H_

// Definitions
#define PAGE_SIZE       64      /* SPM Flash memory page size */
#define RESET_PAGE      0       /* Interrupt vector table address start location */

#ifndef __AVR_ATtiny85__
#define __AVR_ATtiny85__
#pragma message "   >>>   Run, Timonel, run!   <<<   "
#endif

// Optional modules implementation
#define CMD_STPGADDR    false   /* If this is disabled, applications can only be flashed starting    */
                                /* from page 0. This is OK for most applications. It takes ~42 bytes */
                                
#define ENABLE_LED_UI   true    /* If this is enabled, LED_UI_PIN is used to show Timonel status */
                                /* PLEASE DISABLE THIS FOR PRODUCTION! It takes ~30 bytes        */
                                
#define TWO_STEP_INIT   false   /* If this is enabled, Timonel expects a two-step initialization from  */
                                /* an I2C master for starting. Otherwise, single-step init is enough   */

#define APP_USE_TPL_PG  true    /* Allow the application to use trampoline page */
#define AUTO_TPL_CALC   true    /* Auto-trampoline calculation & flash */
                                
// CPU speed
#ifndef F_CPU
#define F_CPU 8000000UL         /* Default CPU speed for delay.h */
#endif

// Led UI
#define LED_UI_PIN      PB1     /* >>> Use PB1 to monitor activity. <<< */
#define LED_UI_DDR      DDRB    /* >>> WARNING! This is not for use <<< */
#define LED_UI_PORT     PORTB   /* >>> in production!               <<< */

// Operation delays
#define TOGGLETIME      0xFFFF  /* LED toggle delay before initialization */
//#define I2CDLYTIME      0x7FFF  /* Main loop times to allow the I2C responses to finish */
#define CYCLESTOEXIT    20      /* Main loop cycles before exit to app if not initialized */

// I2C TX-RX commands data size
#define TXDATASIZE      10      /* TX data size: always even values, min = 2, max = 10 */
#define RXDATASIZE      8       /* RX data size: always even values, min = 2, max = 8 */

// Status byte
#define ST_INIT_1       0       /* Status Bit 1 (1)  : Two-Step Initialization STEP 1 */
#define ST_INIT_2       1       /* Status Bit 2 (2)  : Two-Step Initialization STEP 2 */
#define ST_DEL_FLASH    2       /* Status Bit 3 (4)  : Delete flash memory */
#define ST_APP_READY    3       /* Status Bit 4 (8)  : Application flased OK, ready to run */
#define ST_EXIT_TML     4       /* Status Bit 5 (16) : Exit Timonel & Run Application */
#define ST_BIT_6        5       /* Status Bit 6 (32) : Not used --- (RTR) Ready to Receive ---*/
#define ST_BIT_7        6       /* Status Bit 7 (64) : Not used */
#define ST_BIT_8        7       /* Status Bit 8 (128): Not used */

// Timonel ID characeters
#define ID_CHAR_1       78;     /* N */
#define ID_CHAR_2       66;     /* B */
#define ID_CHAR_3       84;     /* T */

#endif  /* Close ifndef _TML_CONFIG_H_ */
