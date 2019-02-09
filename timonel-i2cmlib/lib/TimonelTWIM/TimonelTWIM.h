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
    word GetAppStart();
    word GetTplAddr();
    byte UploadFirmware(const byte payload[], int payloadsize);
  private:
    byte addr_;
    bool timonel_contacted_ = false;
    bool reusing_twi_connection_ = true;
    byte tml_signature_ = 0;
    word timonel_start_ = 0x0000;
    word application_start_ = 0x0000;
    word trampoline_addr_ = 0x0000;
    word memoryaddr_ = 0x0000;
    byte tml_ver_major_ = 0;
    byte tml_ver_minor_ = 0;
    byte tml_features_code_ = 0;
    byte block_rx_size_ = 0;
    byte GetTmlID();
    void InitTiny(void);
    void TwoStepInit(word time);
    byte WritePageBuff(uint8_t dataArray[]);
    // bool _led_ui_enabled = 0;
    // bool _auto_trampoline_calc = 0;
    // bool _app_use_trampoline_page = 0;
    // bool _allow_set_pageaddr_ = 0;
    // bool _two_step_init_enabled = 0;
    // bool _use_wdt_reset = 0;
    // bool _check_blank_flash = 0;
    // bool _allow_read_flash = 0;

};

#endif /* _TIMONELTWIM_H_ */
