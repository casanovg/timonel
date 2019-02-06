/*
  TimonelTWIM.h
  =============
  Library header for uploading firmware to an Atmel ATTiny85
  microcontroller that runs the Timonel I2C bootloader.
  ---------------------------
  2018-12-13 Gustavo Casanova
  ---------------------------
*/
#ifndef TimonelTWIM_h
#define TimonelTWIM_h

#define USE_SERIAL Serial
#define T_SIGNATURE 84      /* T */ 

#include "Arduino.h"

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
    bool _reusing_twi_connection = true;
    word _timonel_start = 0x0;
    word _trampoline_addr = 0x0;
    byte _block_rx_size = 0;
    byte _version_reply[8] = { 0 };
    byte GetTmlID();
};

#endif
