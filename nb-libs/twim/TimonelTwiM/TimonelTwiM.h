/*
 *  Timonel TWI Master Library
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: TimonelTwiM.h (Header)
 *  ........................................... 
 *  Version: 1.2 / 2019-01-16
 *  gustavo.casanova@nicebots.com
 *  ...........................................
 */

#ifndef _TIMONELTWIM_H_
#define _TIMONELTWIM_H_

#include "../../cmd/nb-twi-cmd.h"
#include "Arduino.h"
#include "NbMicro.h"
#include "Wire.h"
#include "libconfig.h"
#include "stdbool.h"

// Class Timonel: Represents an ATTiny85/45/25 microcontroller running the Timonel bootloader
class Timonel : public NbMicro {
   public:
    Timonel(const byte twi_address = 0, const byte sda = 0, const byte scl = 0);
    typedef struct tml_status_ {
        byte signature = 0;
        byte version_major = 0;
        byte version_minor = 0;
        byte features_code = 0;
        word bootloader_start = 0x0000;
        word application_start = 0x0000;
        word trampoline_addr = 0x0000;
        byte low_fuse = 0;
        byte check_empty_fl = 0;
    } Status;
    Status GetStatus(void);
    byte SetTwiAddress(byte twi_address);
    byte UploadApplication(byte payload[], int payload_size, const int start_address = 0x0000);
    byte RunApplication(void);
    byte DeleteApplication(void);
    byte DumpMemory(const word flash_size = MCU_TOTAL_MEM,
                    const byte rx_packet_size = SLV_PACKET_SIZE,
                    const byte values_per_line = VALUES_PER_LINE);

   private:
    Status status_; /* Goblal struct that holds a Timonel instance's running status */
    byte QueryStatus(void);
    byte BootloaderInit(const word time);
    byte WritePageBuff(const byte data_array[]);
    byte SetPageAddress(const word page_addr);
    byte FillSpecialPage(const byte page_type, const byte app_reset_msb = 0, const byte app_reset_lsb = 0);
    word CalculateTrampoline(const word bootloader_start, const word application_start);
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
