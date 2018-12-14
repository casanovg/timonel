/*
  Timonel.h
  =========
  Library for uploading firmware to an Atmel ATTiny85
  microcontroller that runs the Timonel I2C bootloader.
  2018-12-13 Gustavo Casanova.
*/
#ifndef Timonel_h
#define Timonel_h

#include "Arduino.h"

class Timonel {
  public:
    Timonel(byte address);
    //void dot();
    //void dash();
    byte getTmlVerMaj();
    byte getTmlVerMin();
  private:
    byte _addr;
};

#endif
