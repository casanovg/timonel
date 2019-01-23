/*
  Timonel.h
  =========
  Library for uploading firmware to an Atmel ATTiny85
  microcontroller that runs the Timonel I2C bootloader.
  2018-12-13 Gustavo Casanova.
*/
#ifndef TimonelTWIM_h
#define TimonelTWIM_h

#define USE_SERIAL Serial

#include "Arduino.h"

class Timonel {
  public:
    Timonel(byte twi_address);
    //void dot();
    //void dash();
    byte GetVersionMaj();
    byte GetVersionMin();
  private:
    byte _addr;
    word _timonelStart = 0xFFFF;
    byte _blockRXSize = 0;
    byte _version[8] = { 0 };
    byte GetTmlID();
};

#endif
