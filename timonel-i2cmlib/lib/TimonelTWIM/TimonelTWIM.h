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
    Timonel(byte twi_address, byte sda = 0, byte scl = 0);
    // Struct that holds a Timonel instance's status 
    struct status {
        byte signature = 0;
        byte version_major = 0;
        byte version_minor = 0;
        byte features_code = 0;  
        word bootloader_start = 0x0000;
        word application_start = 0x0000;
        word trampoline_addr = 0x0000;
    };
    struct status GetStatus(void);  
    byte UploadApplication(const byte payload[], int payload_size, int start_address = 0x0000);
    byte RunApplication(void);
    byte DeleteApplication(void);
private:
    byte addr_;
    bool reusing_twi_connection_ = true;
    byte block_rx_size_ = 0;
    struct status status_;
    byte QueryStatus(void);
    void InitTiny(void);
    void TwoStepInit(word time);
    byte WritePageBuff(byte data_array[]);
    byte TWICmdXmit(byte twi_command, byte twi_acknowledge, byte reply_array[] = nullptr, byte reply_size = 0);
    byte TWICmdXmit(byte twi_cmd_array[], byte cmd_size, byte twi_acknowledge, byte reply_array[] = nullptr, byte reply_size = 0);
};

#endif /* _TIMONELTWIM_H_ */

/*
 * Features code settings
 * 
 bool _led_ui_enabled = 0;
 bool _auto_trampoline_calc = 0;
 bool _app_use_trampoline_page = 0;
 bool _allow_set_pageaddr_ = 0;
 bool _two_step_init_enabled = 0;
 bool _use_wdt_reset = 0;
 bool _check_blank_flash = 0;
 bool _allow_read_flash = 0;
 */
