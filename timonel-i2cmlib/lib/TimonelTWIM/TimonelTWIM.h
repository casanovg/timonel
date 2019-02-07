/*
  TimonelTWIM.h
  =============
  Library header for uploading firmware to an Atmel ATTiny85
  microcontroller that runs the Timonel I2C bootloader.
  ---------------------------
  2018-12-13 Gustavo Casanova
  ---------------------------
*/

#ifndef _TIMONELTWIM_H_
#define _TIMONELTWIM_H_

#include "Arduino.h"
#include "Wire.h"
#include <stdbool.h>
#include "tml-twimconfig.h"
#include "nb-i2c-cmd.h"

#define USE_SERIAL Serial
#define V_CMD_LENGTH 9    /* Timonel version reply lenght */
#define T_SIGNATURE 84    /* Timonel signature (expected "T") */
#define V_SIGNATURE 0     /* Ver cmd reply: signature position */
#define V_MAJOR 1         /* Ver cmd reply: major number position */
#define V_MINOR 2         /* Ver cmd reply: minor number position */
#define V_FEATURES 3      /* Ver cmd reply: available features code position */
#define V_BOOT_ADDR_MSB 4 /* Ver cmd reply: Timonel start address MSB position */
#define V_BOOT_ADDR_LSB 5 /* Ver cmd reply: Timonel start address LSB position */
#define V_TMPL_ADDR_MSB 6 /* Ver cmd reply: Trampoline address MSB position */
#define V_TMPL_ADDR_LSB 7 /* Ver cmd reply: Trampoline address LSB position */

// Error GetTmlID (0) 1: No conn
#define ERR_01 0x0        /* Error GetTmlID (0) 1:   */


class Timonel {
  public:
    Timonel(byte twi_address); /* Constructor A */
    Timonel(byte twi_address, byte sda, byte scl); /* Constructor B */
    ~Timonel(); /* Destructor */
    //char signature_char = 0;
    //byte version_major = 0;
    //byte version_minor = 0;
    //byte features = 0;
    char GetSignature();
    bool IsTimonelContacted();
    byte GetVersionMaj();
    byte GetVersionMin();
    byte GetFeatures();
  private:
    byte _addr;
    bool _led_ui_enabled = 0;
    bool _auto_trampoline_calc = 0;
    bool _app_use_trampoline_page = 0;
    bool _allow_set_page_addr = 0;
    bool _two_step_init_enabled = 0;
    bool _use_wdt_reset = 0;
    bool _check_blank_flash = 0;
    bool _allow_read_flash = 0;
    bool _timonel_contacted = false;
    bool _reusing_twi_connection = true;
    word _timonel_start = 0x0;
    word _trampoline_addr = 0x0;
    byte _block_rx_size = 0;
    byte _version_reply[8] = { 0 };
    byte GetTmlID();
};

#endif /* _TIMONELTWIM_H_ */
