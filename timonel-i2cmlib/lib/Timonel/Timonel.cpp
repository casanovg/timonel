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

// Member function to get the Timonel Version Major number
byte Timonel::getVersionMaj() {
  return(1);
}

// Member function to get the Timonel Version Minor number
byte Timonel::getVersionMin() {
  return(2);
}

// Member function to get the Timonel available features code
byte Timonel::getFeatures() {
  byte cmdTX[1] = { GETTMNLV };
    byte txSize = sizeof(cmdTX);
    USE_SERIAL.printf_P("\n\rGet Timonel Version >>> %d (GETTMNLV)\n\r", cmdTX[0]);
    byte transmitData[1] = { 0 };
    for (byte i = 0; i < txSize; i++) {
      transmitData[i] = cmdTX[i];
      Wire.beginTransmission(_addr);
      Wire.write(transmitData[i]);
      Wire.endTransmission();
    }
    // Receive acknowledgement
    _blockRXSize = Wire.requestFrom(_addr, (byte)9);
    byte ackRX[9] = { 0 };   // Data received from slave
    for (int i = 0; i < _blockRXSize; i++) {
      ackRX[i] = Wire.read();
    }
    if (ackRX[0] == ACKTMNLV) {
      _timonelStart = (ackRX[5] << 8) + ackRX[6];
      word trampolineJump = (~(((ackRX[7] << 8) | ackRX[8]) & 0xFFF));
    	trampolineJump++;
    	trampolineJump = ((((_timonelStart >> 1) - trampolineJump) & 0xFFF) << 1);
      USE_SERIAL.printf_P(" ____________________________________");
      USE_SERIAL.printf_P("| ");
      if (ackRX[1] == 84) {
        USE_SERIAL.printf_P("| Timonel Bootloader v");
      }
  		USE_SERIAL.printf_P("%d.%d", ackRX[2], ackRX[3]);
      switch (ackRX[2]) {
        case 0: {
          USE_SERIAL.printf_P(" Pre-release ");
          break;
        }
        case 1: {
          USE_SERIAL.printf_P(" \"Sandra\" ");
          break;
        }
        default: {
          USE_SERIAL.printf_P(" Unknown ");
          break;
        }
      }
      USE_SERIAL.printf_P("| ================================");
      USE_SERIAL.printf_P("| Bootloader address: 0x%04X\n\r", _timonelStart);
		  USE_SERIAL.printf_P("|  Application start: %02X%02X\n\r", ackRX[8], ackRX[7]);
      if ((ackRX[8] == 0xFF) && (ackRX[7] == 0xFF)) {
        USE_SERIAL.printf_P(" (Not Set)");
      }
      else {
        USE_SERIAL.printf_P(" (0x%04X)\n\r", trampolineJump);
      }
  		USE_SERIAL.printf_P("|      Features Code: %d\n\r", ackRX[4]);
      USE_SERIAL.printf_P(" ____________________________________\n\r");
    }
    else {
      USE_SERIAL.printf_P("ESP8266 - Error parsing %d command! <<< %d\n\r", cmdTX[0], ackRX[0]);
    }
    return 0;
}
