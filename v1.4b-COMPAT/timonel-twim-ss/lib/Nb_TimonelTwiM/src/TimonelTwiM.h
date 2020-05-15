/*
 *  Timonel TWI Master Library
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: TimonelTwiM.h (Header)
 *  ........................................... 
 *  Version: 1.0.1 / 2020-05-07
 *  gustavo.casanova@gmail.com
 *  ...........................................
 *  This library enables uploading firmware to a microcontroller
 *  running the Timonel bootloader. It inherits from the NbMicro
 *  class to implement the NB command set over the TWI (I2C) bus.
 */

#ifndef _TIMONELTWIM_H_
#define _TIMONELTWIM_H_

#include <Arduino.h>
#include <NbMicro.h>

#include "libconfig.h"

// Class Timonel: Represents an ATTiny85/45/25 microcontroller running the Timonel bootloader
class Timonel : public NbMicro {
   public:
    Timonel(const byte twi_address = 0, const byte sda = 0, const byte scl = 0);
    ~Timonel();
    typedef struct tml_status_ {
        byte signature = 0;
        byte version_major = 0;
        byte version_minor = 0;
        byte features_code = 0;
        byte ext_features_code = 0;
        word bootloader_start = 0;
        word application_start = 0;
        word trampoline_addr = 0;
        byte low_fuse_setting = 0;
        byte oscillator_cal = 0;
        byte check_empty_fl = 0;
    } Status;
    Status GetStatus(void);
    byte SetTwiAddress(byte twi_address);
    byte RunApplication(void);
    byte DeleteApplication(void);
    byte UploadApplication(byte payload[],
                           int payload_size,
                           const int start_address = 0);
#if ((defined FEATURES_CODE) && ((FEATURES_CODE >> F_CMD_READFLASH) & true))
    byte DumpMemory(const word flash_size = MCU_TOTAL_MEM,
                    const byte rx_packet_size = SLV_PACKET_SIZE,
                    const byte values_per_line = VALUES_PER_LINE);
#endif /* FEATURES_CODE >> F_CMD_READFLASH */

   private:
    Status status_; /* Global struct that holds a Timonel instance's running status */
    byte BootloaderInit(void);
    byte QueryStatus(void);
    byte SendDataPacket(const byte data_packet[]);
#if (!((defined FEATURES_CODE) && ((FEATURES_CODE >> F_AUTO_PAGE_ADDR) & true)))
    byte SetPageAddress(const word page_addr);
    byte FillSpecialPage(const byte page_type,
                         const byte app_reset_msb = 0,
                         const byte app_reset_lsb = 0);
    word CalculateTrampoline(const word bootloader_start,
                             const word application_start);
#endif /* FEATURES_CODE >> F_CMD_SETPGADDR */
};

#endif /* _TIMONELTWIM_H_ */
