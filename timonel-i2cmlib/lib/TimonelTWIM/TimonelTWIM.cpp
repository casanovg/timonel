/*
  TimonelTWIM.cpp
  ===============
  Library code for uploading firmware to an Atmel ATTiny85
  microcontroller that runs the Timonel I2C bootloader.
  ---------------------------
  2018-12-13 Gustavo Casanova
  ---------------------------
*/

#include "TimonelTWIM.h"

// Constructor A (use it when a TWI channel is already opened)
Timonel::Timonel(byte twi_address) {
  _addr = twi_address;
  if(GetTmlID() == 0) {
    _timonel_contacted = true;
    _reusing_twi_connection = true;
  }
  else {
    //delete this;  /* If the I2C device is not a Timonel, destroy the object ... */
  }
}

// Constructor B (use it to open the TWI channel)
Timonel::Timonel(byte twi_address, byte sda, byte scl) {
  _addr = twi_address;
  Wire.begin(sda, scl); /* Init I2C sda:GPIO0, scl:GPIO2 (ESP-01) / sda:D3, scl:D4 (NodeMCU) */
  if(GetTmlID() == 0) {
    _timonel_contacted = true;
    _reusing_twi_connection = false;
  }
  else {
    //delete this;  /* If the I2C device is not a Timonel, destroy the object ... */    
  }
}

// Destructor
Timonel::~Timonel() {
  if(_reusing_twi_connection == true) {
    //USE_SERIAL.printf_P("\n\r[Class Destructor] Reused I2C connection will remain active ...\n\r");
  }
  else {
    //USE_SERIAL.printf_P("\n\r[Class Destructor] The I2C connection created by this object will be closed ...\n\r");
  }
}

// Member function to know if Timonel was contacted
bool Timonel::IsTimonelContacted() {
  return(_timonel_contacted);
}

// Member function to get the Timonel version major number
byte Timonel::GetVersionMaj() {
  return(_version_reply[1]);
}

// Member function to get the Timonel version minor number
byte Timonel::GetVersionMin() {
  return(_version_reply[2]);
}

// Member function to get the available features
byte Timonel::GetFeatures() {
  return(_version_reply[3]);
}

// Member function to get the Timonel available features code
byte Timonel::GetTmlID() {
  // I2C TX
  Wire.beginTransmission(_addr);
  Wire.write(GETTMNLV);
  Wire.endTransmission(_addr);
  // I2X RX
  _block_rx_size = Wire.requestFrom(_addr, (byte)9, true);
  byte ackRX[9] = { 0 };  /* Data received from I2C slave */
  for (int i = 0; i < _block_rx_size; i++) {
    ackRX[i] = Wire.read();
  }
  if (ackRX[0] == ACKTMNLV) {
    if (ackRX[1] == T_SIGNATURE) {
      _timonel_start = (ackRX[5] << 8) + ackRX[6];
      _trampoline_addr = (~(((ackRX[7] << 8) | ackRX[8]) & 0xFFF));
      _trampoline_addr++;
      _trampoline_addr = ((((_timonel_start >> 1) - _trampoline_addr) & 0xFFF) << 1);
      _version_reply[0] = ackRX[1]; /* Signature */
      _version_reply[1] = ackRX[2]; /* Timonel version major */
      _version_reply[2] = ackRX[3]; /* Timonel version minor */
      _version_reply[3] = ackRX[4]; /* Features code */
      _version_reply[4] = ackRX[5]; /* Timonel address MSB */
      _version_reply[5] = ackRX[6]; /* Timonel address LSB */
      _version_reply[6] = ackRX[7]; /* Trampoline address MSB */
      _version_reply[7] = ackRX[8]; /* Trampoline address LSB */
    }
    else {
      //USE_SERIAL.printf_P("\n\r[Timonel::GetTmlID] Error: Firmware signature unknown!\n\r");
      return(2);
    }
  }
  else {
    //USE_SERIAL.printf_P("\n\r[Timonel::GetTmlID] Error: parsing %d command! <<< %d\n\r", GETTMNLV, ackRX[0]);
    return(1);
  }
  return(0);
}

// Member function to get the Timonel available features code
// byte Timonel::GetTmlID() {
//   // I2C TX
//   Wire.beginTransmission(_addr);
//   Wire.write(GETTMNLV);
//   Wire.endTransmission();
//   // I2X RX
//   _block_rx_size = Wire.requestFrom(_addr, (byte)9);
//   byte ackRX[9] = { 0 };   // Data received from slave
//   for (int i = 0; i < _block_rx_size; i++) {
//     ackRX[i] = Wire.read();
//   }
//   if (ackRX[0] == ACKTMNLV) {
//     _timonel_start = (ackRX[5] << 8) + ackRX[6];
//     _trampoline_addr = (~(((ackRX[7] << 8) | ackRX[8]) & 0xFFF));
//     _trampoline_addr++;
//     _trampoline_addr = ((((_timonel_start >> 1) - _trampoline_addr) & 0xFFF) << 1);

//    USE_SERIAL.write(27);       // ESC command
// 	  USE_SERIAL.printf_P("[2J");    // clear screen command
// 	  USE_SERIAL.write(27);       // ESC command
// 	  USE_SERIAL.printf_P("[H");     // cursor to home command

//     if (ackRX[1] == 84) { /* T */
//       //USE_SERIAL.printf_P("\n\n\r| ================================\n\r");
//       USE_SERIAL.printf_P("\n\r ____________________________________\n\n\r");
//       USE_SERIAL.printf_P("| Timonel Bootloader v");
//       USE_SERIAL.printf_P("%d.%d", ackRX[2], ackRX[3]);
//       switch (ackRX[2]) {
//         case 0: {
//           USE_SERIAL.printf_P(" Pre-release \n\r");
//             break;
//           }
//           case 1: {
//             USE_SERIAL.printf_P(" \"Sandra\" \n\r");
//             break;
//           }
//           default: {
//             USE_SERIAL.printf_P(" Unknown \n\r");
//             break;
//           }
//       }
//       USE_SERIAL.printf_P("| Bootloader address: 0x%04X\n\r", _timonel_start);
//       USE_SERIAL.printf_P("|  Application start: %02X%02X", ackRX[8], ackRX[7]);
//       if ((ackRX[8] == 0xFF) && (ackRX[7] == 0xFF)) {
//         USE_SERIAL.printf_P(" (Not Set)\n\r");
//       }
//       else {
//         USE_SERIAL.printf_P(" (0x%04X)\n\r", _trampoline_addr);
//       }
//       USE_SERIAL.printf_P("|      Features Code: %d\n\r", ackRX[4]);
//       USE_SERIAL.printf_P(" ____________________________________\n\r");
//     }
//     else {
//       USE_SERIAL.printf_P("\n\n\rWarning: Firmware Unknown ...\n\r");
//     }
//   }
//   else {
//     USE_SERIAL.printf_P("ESP8266 - Error parsing %d command! <<< %d\n\r", GETTMNLV, ackRX[0]);
//   }
//   return 0;
// }
