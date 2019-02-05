/*
  TimonelTWIM.cpp
  ===============
  Library code for uploading firmware to an Atmel ATTiny85
  microcontroller that runs the Timonel I2C bootloader.
  ---------------------------
  2018-12-13 Gustavo Casanova
  ---------------------------
*/

#include "Arduino.h"
#include "TimonelTWIM.h"
#include "Wire.h"
#include "nb-i2c-cmd.h"

#define USE_SERIAL Serial

// Timonel class constructor A (use it when a TWI channel is already opened)
Timonel::Timonel(byte twi_address) {
  //_addr = twi_address;
   GetTmlID();
}

// Timonel class constructor B (use it to open the TWI channel)
Timonel::Timonel(byte twi_address, byte sda, byte scl) {
  _addr = twi_address;
  //_sda = sda;
  //_scl = scl;
  Wire.begin(sda, scl);   /* Init I2C sda:GPIO0, scl:GPIO2 (ESP-01) / sda:D3, scl:D4 (NodeMCU) */
  GetTmlID();
}

// Member function to get the Timonel Version Major number
byte Timonel::GetVersionMaj() {
  return(1);
}

// Member function to get the Timonel Version Minor number
byte Timonel::GetVersionMin() {
  return(2);
}

// Member function to get the Timonel available features code
byte Timonel::GetTmlID() {
  // I2C TX
  Wire.beginTransmission(_addr);
  Wire.write(GETTMNLV);
  Wire.endTransmission();
  // I2X RX
  _block_rx_size = Wire.requestFrom(_addr, (byte)9);
  byte ackRX[9] = { 0 };   // Data received from slave
  for (int i = 0; i < _block_rx_size; i++) {
    ackRX[i] = Wire.read();
  }
  if (ackRX[0] == ACKTMNLV) {
    _timonel_start = (ackRX[5] << 8) + ackRX[6];
    _trampoline_addr = (~(((ackRX[7] << 8) | ackRX[8]) & 0xFFF));
    _trampoline_addr++;
    _trampoline_addr = ((((_timonel_start >> 1) - _trampoline_addr) & 0xFFF) << 1);

    Serial.write(27);       // ESC command
	  Serial.printf_P("[2J");    // clear screen command
	  Serial.write(27);       // ESC command
	  Serial.printf_P("[H");     // cursor to home command

    if (ackRX[1] == 84) { /* T */
      //USE_SERIAL.printf_P("\n\n\r| ================================\n\r");
      USE_SERIAL.printf_P("\n\r ____________________________________\n\n\r");
      USE_SERIAL.printf_P("| Timonel Bootloader v");
      USE_SERIAL.printf_P("%d.%d", ackRX[2], ackRX[3]);
      switch (ackRX[2]) {
        case 0: {
          USE_SERIAL.printf_P(" Pre-release \n\r");
            break;
          }
          case 1: {
            USE_SERIAL.printf_P(" \"Sandra\" \n\r");
            break;
          }
          default: {
            USE_SERIAL.printf_P(" Unknown \n\r");
            break;
          }
      }
      USE_SERIAL.printf_P("| Bootloader address: 0x%04X\n\r", _timonel_start);
      USE_SERIAL.printf_P("|  Application start: %02X%02X", ackRX[8], ackRX[7]);
      if ((ackRX[8] == 0xFF) && (ackRX[7] == 0xFF)) {
        USE_SERIAL.printf_P(" (Not Set)\n\r");
      }
      else {
        USE_SERIAL.printf_P(" (0x%04X)\n\r", _trampoline_addr);
      }
      USE_SERIAL.printf_P("|      Features Code: %d\n\r", ackRX[4]);
      USE_SERIAL.printf_P(" ____________________________________\n\r");
    }
    else {
      USE_SERIAL.printf_P("\n\n\rWarning: Firmware Unknown ...\n\r");
    }
  }
  else {
    USE_SERIAL.printf_P("ESP8266 - Error parsing %d command! <<< %d\n\r", GETTMNLV, ackRX[0]);
  }
  return 0;
}
