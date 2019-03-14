/*
  NbMicro.h
  =========
  Library header for TWI (I2C) communications with
  a microcontroller using the NB command set.
  ---------------------------
  2019-03-12 Gustavo Casanova
  ---------------------------
*/

#ifndef _NBMICRO_H_
#define _NBMICRO_H_

#include <unordered_set>
#include "../../../command-set/nb-twi-cmd.h"
#include "Arduino.h"
#include "Wire.h"
#include "stdbool.h"
//#include <iostream>
//#include <exception>

#define USE_SERIAL Serial
#define MAX_TWI_DEVS 28
#define MIN_TWI_ADDR 8
#define MAX_TWI_ADDR 63

static std::unordered_set<byte> in_use;

// Class NbMicro: Represents a microcontroller using the NB command set connected to the TWI bus
class NbMicro {
   public:
    NbMicro(byte twi_address = 0, byte sda = 0, byte scl = 0);
    byte SetTwiAddress(byte twi_address);
    byte TwiCmdXmit(byte twi_cmd, byte twi_reply,
                    byte twi_reply_arr[] = nullptr, byte reply_size = 0);
    byte TwiCmdXmit(byte twi_cmd_arr[], byte cmd_size, byte twi_reply,
                    byte twi_reply_arr[] = nullptr, byte reply_size = 0);

   protected:
    ~NbMicro();
    byte InitMicro(void);
    byte addr_ = 0, sda_ = 0, scl_ = 0;
    bool reusing_twi_connection_ = true;
    //std::unordered_set<byte> in_use;
    //byte addr_pool_[28] = { 0 };
   private:
};

// Class TwiBus: Represents a Two Wire Interfase (I2C) bus
class TwiBus {
   public:
    typedef struct device_info_ {
        byte addr = 0;
        String firmware = "";
        byte version_major = 0;
        byte version_minor = 0;
    } DeviceInfo;
    TwiBus(byte sda = 0, byte scl = 0);
    byte ScanBus(bool *p_app_mode = nullptr);
    byte ScanBus(DeviceInfo dev_info_arr[] = nullptr, byte arr_size = MAX_TWI_DEVS,
                 byte start_twi_addr = MIN_TWI_ADDR);
    //byte GetAllTimonels(Timonel tml_arr[], byte tml_arr_size);
   private:
    byte sda_ = 0, scl_ = 0;
    bool reusing_twi_connection_ = true;
};

#endif /* _NBMICRO_H_ */
