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
    Timonel(const uint8_t twi_address = 0, const uint8_t sda = 0, const uint8_t scl = 0);
    ~Timonel();
    typedef struct tml_status_ {
        uint8_t signature = 0;
        uint8_t version_major = 0;
        uint8_t version_minor = 0;
        uint8_t features_code = 0;
        uint8_t ext_features_code = 0;
        uint16_t bootloader_start = 0;
        uint16_t application_start = 0;
        uint16_t trampoline_addr = 0;
        uint8_t low_fuse_setting = 0;
        uint8_t oscillator_cal = 0;
        uint8_t check_empty_fl = 0;
    } Status;
    Status GetStatus(void);
    uint8_t SetTwiAddress(uint8_t twi_address);
    uint8_t RunApplication(void);
    uint8_t DeleteApplication(void);
    uint8_t UploadApplication(uint8_t payload[],
                           uint16_t payload_size,
                           const uint16_t start_address = 0);
#if ((defined FEATURES_CODE) && ((FEATURES_CODE >> F_CMD_READFLASH) & true))
    uint8_t DumpMemory(const uint16_t flash_size = MCU_TOTAL_MEM,
                    const uint8_t rx_packet_size = SLV_PACKET_SIZE,
                    const uint8_t values_per_line = VALUES_PER_LINE);
#endif /* FEATURES_CODE >> F_CMD_READFLASH */

   private:
    Status status_; /* Global struct that holds a Timonel instance's running status */
    uint8_t BootloaderInit(void);
    uint8_t QueryStatus(void);
    uint8_t SendDataPacket(const uint8_t data_packet[]);
#if (!((defined FEATURES_CODE) && ((FEATURES_CODE >> F_AUTO_PAGE_ADDR) & true)))
    uint8_t SetPageAddress(const uint16_t page_addr);
    uint8_t FillSpecialPage(const uint8_t page_type,
                         const uint8_t app_reset_msb = 0,
                         const uint8_t app_reset_lsb = 0);
    uint16_t CalculateTrampoline(const uint16_t bootloader_start,
                             const uint16_t application_start);
#endif /* FEATURES_CODE >> F_CMD_SETPGADDR */
};

#endif /* _TIMONELTWIM_H_ */
