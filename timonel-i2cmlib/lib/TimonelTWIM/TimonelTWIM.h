/*
  TimonelTwiM.h
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
#include "libconfig.h"
#include "../../include/nb-i2c-cmd.h"
#include "../NBTinyX5/NBTinyX5.h"

// Class Timonel: Represents an ATTiny85/45/25 microcontroller running the Timonel bootloader
class Timonel: public NbTinyX5 {
public:
    Timonel(byte twi_address = 0, byte sda = 0, byte scl = 0);
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
    byte UploadApplication(byte payload[], int payload_size, int start_address = 0x0000);
    byte RunApplication(void);
    byte DeleteApplication(void);
    byte DumpMemory(word flash_size = MCU_TOTAL_MEM, byte rx_data_size = RX_DATA_SIZE, byte values_per_line = VALUES_PER_LINE);
    //byte SetTwiAddress(byte twi_address);
private:
    byte block_rx_size_ = 0;
    struct status status_;
    byte QueryStatus(void);
    byte TwoStepInit(word time);
    byte WritePageBuff(byte data_array[]);
    byte SetPageAddress(word page_addr);
    byte FillSpecialPage(byte page_type, byte app_reset_msb = 0, byte app_reset_lsb = 0);
    word CalculateTrampoline(word bootloader_start, word application_start);
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
