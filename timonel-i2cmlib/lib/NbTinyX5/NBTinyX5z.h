/*
  NBTinyX5.h
  ==========
  Library header for I2C communications with an Atmel
  ATTiny85 microcontroller using the NB protocol.
  ---------------------------
  2019-03-08 Gustavo Casanova
  ---------------------------
*/

#ifndef _NBTINYX5_H_
#define _NBTINYX5_H_

#include "Arduino.h"
#include "Wire.h"
#include "stdbool.h"
#include "../../include/nb-i2c-cmd.h"
//#include "../TimonelTwiM/TimonelTwiM.h"
#include <unordered_set>

#define USE_SERIAL Serial

static std::unordered_set<byte> in_use;

// Class NbTinyX5: Represents a generic ATTiny85/45/25 microcontroller
class NbTinyX5 {
public:
    NbTinyX5(byte twi_address = 0, byte sda = 0, byte scl = 0);
    ~NbTinyX5();
    byte TwiCmdXmit(byte twi_cmd, byte twi_reply, byte twi_reply_arr[] = nullptr, byte reply_size = 0);
    byte TwiCmdXmit(byte twi_cmd_arr[], byte cmd_size, byte twi_reply, byte twi_reply_arr[] = nullptr, byte reply_size = 0);
    byte SetTwiAddress(byte twi_address);
protected:
    byte addr_ = 0, sda_ = 0, scl_ = 0;
    bool reusing_twi_connection_ = true;
    byte InitTiny(void);
    //std::unordered_set<byte> in_use;
    //byte addr_pool_[28] = { 0 };
private:
};

// Class TwiBus: Represents a Two Wire Interfase (I2C) bus
class TwiBus {
public:
struct device_info {
    byte addr = 0;
    String firmware = "";
    byte version_major = 0;
    byte version_minor = 0;
};
    TwiBus(byte sda = 0, byte scl = 0);
    byte ScanBus(bool *p_app_mode = nullptr); /* Returns the TWI address of the first device found */
    byte ScanBus(struct device_info dev_info_arr[] = nullptr, byte arr_size = 28, byte start_twi_addr = 8);    /*Returns an array with all TWI devices found (address, firmware, version) */
    //byte GetAllTimonels(Timonel tml_arr[], byte tml_arr_size);
private:
    byte sda_ = 0, scl_ = 0;
    bool reusing_twi_connection_ = true; 
};

#endif /* _NBTINYX5_H_ */