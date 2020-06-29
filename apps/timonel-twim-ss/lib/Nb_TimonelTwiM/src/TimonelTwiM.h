/*
 *  Timonel TWI Master Library
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: TimonelTwiM.h (Header)
 *  ........................................... 
 *  Version: 1.2.0 / 2020-06-22
 *  gustavo.casanova@gmail.com
 *  ...........................................
 *  This library enables uploading firmware to a microcontroller
 *  running the Timonel bootloader. It inherits from the NbMicro
 *  class to implement the NB command set over the TWI (I2C) bus.
 */

#ifndef TIMONELTWIM_H
#define TIMONELTWIM_H

#include <Arduino.h>
#include <NbMicro.h>

#include "libconfig.h"

// Class Timonel: Represents an ATTiny85/45/25 microcontroller running the Timonel bootloader
class Timonel : public NbMicro {
   public:
    Timonel(const uint8_t twi_address = 0, const uint8_t sda = 0, const uint8_t scl = 0);
    ~Timonel();
    typedef struct tml_status_ {
        uint8_t signature = 0;           // Bootloader signature
        uint8_t version_major = 0;       // Bootloader version major number
        uint8_t version_minor = 0;       // Bootloader version minor number
        uint8_t features_code = 0;       // Available features compiled
        uint8_t ext_features_code = 0;   // Available extended features compiled
        uint16_t bootloader_start = 0;   // Bootloader start memory position
        uint16_t application_start = 0;  // Trampoline bytes pointing to application launch, if existing
        uint8_t low_fuse_setting = 0;    // Low fuse bits
        uint8_t oscillator_cal = 0;      // Internal RC oscillator calibration
        // uint16_t trampoline_addr = 0; // Application start absolute address (deprecated from v1.5)
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
#endif  // FEATURES_CODE >> F_CMD_READFLASH
#if ((defined EXT_FEATURES) && ((EXT_FEATURES >> F_CMD_READDEVS) & true))
    typedef struct device_settings_ {
        uint8_t low_fuse_bits;       // Low fuse bits
        uint8_t high_fuse_bits;      // High fuse bits
        uint8_t extended_fuse_bits;  // Extended fuse bits
        uint8_t lock_bits;           // Device lock bits
        uint8_t signature_byte_0;    // Device signature byte 0
        uint8_t signature_byte_1;    // Device signature byte 1
        uint8_t signature_byte_2;    // Device signature byte 2
        uint8_t calibration_0;       // Calibration data for internal oscillator at 8.0 MHz
        uint8_t calibration_1;       // Calibration data for internal oscillator at 6.4 MHz
    } DevSettings;
    DevSettings GetDevSettings(void);
#endif  // EXT_FEATURES >> F_CMD_READDEVS
#if ((defined EXT_FEATURES) && ((EXT_FEATURES >> E_EEPROM_ACCESS) & true))
        bool WriteEeprom(const uint16_t eeprom_addr, uint8_t data_byte);
        uint8_t ReadEeprom(const uint16_t eeprom_addr);
#endif   // EXT_FEATURES >> E_EEPROM_ACCESS

   private:
    Status status_;  // Global struct that holds a Timonel instance's running status */
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
#endif  // FEATURES_CODE >> F_CMD_SETPGADDR
};

#endif  // TIMONELTWIM_H
