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
#include "stdbool.h"
#include "tml-twimconfig.h"
#include "nb-i2c-cmd.h"

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
    word GetTmlStart();
    word GetTrampoline();    
  private:
    byte _addr;
    bool _timonel_contacted = false;
    bool _reusing_twi_connection = true;
    byte _tml_signature = 0;
    word _timonel_start = 0x0000;
    word _trampoline_addr = 0x0000;
    word _memory_addr = 0x0000;
    byte _tml_ver_major = 0;
    byte _tml_ver_minor = 0;
    byte _tml_features_code = 0;
    byte _block_rx_size = 0;
    byte GetTmlID();

    // bool _led_ui_enabled = 0;
    // bool _auto_trampoline_calc = 0;
    // bool _app_use_trampoline_page = 0;
    // bool _allow_set_page_addr = 0;
    // bool _two_step_init_enabled = 0;
    // bool _use_wdt_reset = 0;
    // bool _check_blank_flash = 0;
    // bool _allow_read_flash = 0;

};

#endif /* _TIMONELTWIM_H_ */
