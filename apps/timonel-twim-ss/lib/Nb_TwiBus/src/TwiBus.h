/*
 *  TWI Bus Master Library
 *  Author: Gustavo Casanova
 *  ...........................................
 *  File: TwiBus.h (Header)
 *  ........................................... 
 *  Version: 1.1.0 / 2020-05-25
 *  gustavo.casanova@gmail.com
 *  ...........................................
 *  This library allows scanning the TWI (I2C) bus in search
 *  of connected devices addresses and data. If a device found
 *  is running Timonel, it returns its version number.
 */

#ifndef _TWIBUS_H_
#define _TWIBUS_H_

#include <Arduino.h>
#include <Wire.h>

#include "libconfig.h"

#if DETECT_TIMONEL
#include <TimonelTwiM.h>
#endif  // DETECT_TIMONEL

/* 
 * ===================================================================
 * TwiBus class: Represents a Two Wire Interfase (I2C) bus
 * ===================================================================
 */
class TwiBus {
   public:
    typedef struct device_info_ {
        uint8_t addr = 0;
        String firmware = "";
        uint8_t version_major = 0;
        uint8_t version_minor = 0;
    } DeviceInfo;
    TwiBus(uint8_t sda = 0, uint8_t scl = 0);
    ~TwiBus();
    uint8_t ScanBus(bool *p_app_mode = nullptr);
    uint8_t ScanBus(DeviceInfo dev_info_arr[],
                 uint8_t arr_size = HIG_TWI_ADDR + 1,
                 uint8_t start_twi_addr = LOW_TWI_ADDR);

   private:
    uint8_t sda_ = 0, scl_ = 0;
};

#endif /* _TWIBUS_H_ */
