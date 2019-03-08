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

#define USE_SERIAL Serial

class NBTinyX5 {
public:
    NBTinyX5(byte twi_address, byte sda = 0, byte scl = 0);
    byte ScanTWI(void);
protected:
    byte addr_ = 0, sda_ = 0, scl_ = 0;
    bool reusing_twi_connection_ = true;
    byte TwiCmdXmit(byte twi_cmd, byte twi_reply, byte twi_reply_arr[] = nullptr, byte reply_size = 0);
    byte TwiCmdXmit(byte twi_cmd_arr[], byte cmd_size, byte twi_reply, byte twi_reply_arr[] = nullptr, byte reply_size = 0);
private:    
};

#endif /* _NBTINYX5_H_ */