/*
  Timonel.cpp
  ===========
  Library for uploading firmware to an Atmel ATTiny85
  microcontroller that runs the Timonel I2C bootloader.
  2018-12-13 Gustavo Casanova.
*/

#include "Arduino.h"
#include "Timonel.h"
#include "Wire.h"
#include "nb-i2c-cmd.h"

// Timonel class constructor
Timonel::Timonel(byte address) {
  _addr = address;
}

// Method to get the Timonel Version Major number
byte Timonel::getTmlVerMaj() {
  return(1);
}

// Method to get the Timonel Version Minor number
byte Timonel::getTmlVerMin() {
  return(2);
}
