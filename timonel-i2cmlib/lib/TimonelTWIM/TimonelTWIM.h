/*
  TimonelTWIM.h
  =============
  Library header for uploading firmware to an Atmel ATTiny85
  microcontroller that runs the Timonel I2C bootloader.
  ---------------------------
  2018-12-13 Gustavo Casanova
  ---------------------------
*/
#ifndef TimonelTWIM_h
#define TimonelTWIM_h

#define USE_SERIAL Serial

#include "Arduino.h"

class Timonel {
  public:
    Timonel(byte twi_address); /* Constructor A */
    Timonel(byte twi_address, byte sda, byte scl); /* Constructor B */
    //void dot();
    //void dash();
    byte GetVersionMaj();
    byte GetVersionMin();
  private:
    byte _addr;
    byte _sda;
    byte _scl;
    word _timonelStart = 0xFFFF;
    byte _blockRXSize = 0;
    byte _version[8] = { 0 };
    byte GetTmlID();
};

#endif
