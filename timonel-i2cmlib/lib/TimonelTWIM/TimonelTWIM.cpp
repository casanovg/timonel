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

// Constructor A (Use it when a TWI channel is already opened)
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

// Constructor B (Use it to open the TWI channel)
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

// Function to know if Timonel was contacted
bool Timonel::IsTimonelContacted() {
  return(_timonel_contacted);
}

// Function to get the Timonel version major number
byte Timonel::GetVersionMaj() {
  return(_tml_ver_major);
}

// Function to get the Timonel version minor number
byte Timonel::GetVersionMin() {
  return(_tml_ver_minor);
}

// Function to get the available features
byte Timonel::GetFeatures() {
  return(_tml_features_code);
}

// Function to get the Timonel bootloader start address
word Timonel::GetTmlStart() {
  return(_timonel_start);
}

// Function to get the tranpoline address
word Timonel::GetTrampoline() {
  return(_trampoline_addr);
}

// Function to get the Timonel available features code
byte Timonel::GetTmlID() {
  // I2C TX
  Wire.beginTransmission(_addr);
  Wire.write(GETTMNLV);
  Wire.endTransmission(_addr);
  // I2X RX
  _block_rx_size = Wire.requestFrom(_addr, (int)V_CMD_LENGTH, (int)true);
  byte ackRX[V_CMD_LENGTH] = { 0 };  /* Data received from I2C slave */
  for (int i = 0; i < _block_rx_size; i++) {
    ackRX[i] = Wire.read();
  }
  if (ackRX[CMD_ACK_POS] == ACKTMNLV) {
    if (ackRX[V_SIGNATURE] == T_SIGNATURE) {
      _timonel_start = (ackRX[V_BOOT_ADDR_MSB] << 8) + ackRX[V_BOOT_ADDR_LSB];
      _trampoline_addr = (~(((ackRX[V_TMPL_ADDR_MSB] << 8) | ackRX[V_TMPL_ADDR_LSB]) & 0xFFF));
      _trampoline_addr++;
      _trampoline_addr = ((((_timonel_start >> 1) - _trampoline_addr) & 0xFFF) << 1);
      _tml_signature = ackRX[V_SIGNATURE];
      _tml_ver_major = ackRX[V_MAJOR];
      _tml_ver_minor = ackRX[V_MINOR];
      _tml_features_code = ackRX[V_FEATURES];
      
      //_version_reply[V_BOOT_ADDR_MSB] = ackRX[V_BOOT_ADDR_MSB + 1];
      //_version_reply[V_BOOT_ADDR_LSB] = ackRX[V_BOOT_ADDR_LSB + 1];
      //_version_reply[V_TMPL_ADDR_MSB] = ackRX[V_TMPL_ADDR_MSB + 1];
      //_version_reply[V_TMPL_ADDR_LSB] = ackRX[V_TMPL_ADDR_LSB + 1];
    }
    else {
      //USE_SERIAL.printf_P("\n\r[Timonel::GetTmlID] Error: Firmware signature unknown!\n\r");
      return(ERR_02);
    }
  }
  else {
    //USE_SERIAL.printf_P("\n\r[Timonel::GetTmlID] Error: parsing %d command! <<< %d\n\r", GETTMNLV, ackRX[0]);
    return(ERR_01);
  }
  return(OK);
}
