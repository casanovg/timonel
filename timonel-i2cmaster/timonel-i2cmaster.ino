// ********************************************************
// *  Timonel I2C Master                                  *
// *  ==================                                  *
// *  I2C Master for Bootloader Tests                     *
// *  ..................................................  *
// *  Author: Gustavo Casanova                            *
// *  ..................................................  *
// *  Firmware Version: 1.1 | MCU: ESP8266                *
// *  2018-10-29 gustavo.casanova@nicebots.com            *
// ********************************************************
//
// Run timonel-i2cmaster on a NodeMCU, ESP-01 or ESP-12 Module
// Run timonel-bootloader on a Digispark or ATtiny85
//
// Basic command path to Attiny85:
// -------------------------------
// User (serial console) --> ESP8266 --> Attiny85
//
// Available commands (test app mode):
// -----------------------------------
// a - (STDPB1_1) Set ATtiny85 PB1 = 1
// s - (STDPB1_0) Set ATtiny85 PB1 = 0
// z - (RESTART-) Reboot ESP-8266 and initialize ATtiny85
// x - (RESETINY) Reset ATtiny85
// ? - (HELP) Command help

#define ESP8266      true  /* True = ESP8266, False = Arduino */

#include <Wire.h>
#include "nb-i2c-cmd.h"
#include "nb-i2c-crc.h"
#if ESP8266
#include <pgmspace.h>
#endif /* ESP8266 */
#include "Payloads/payload.h"

// Timonel bootloader
#define MCUTOTALMEM   8192  /* Slave MCU total flash memory*/
#define MAXCKSUMERRORS  100   /* Max number of checksum errors allowed in bootloader comms */
#define TXDATASIZE    8   /* TX data size for WRITBUFF command */
#define FLASHPGSIZE   64    /* Tiny85 flash page buffer size */
#define DATATYPEBYTE  1   /* Buffer data type "Byte" */

// Global Variables
byte slaveAddress = 0;
byte blockRXSize = 0;
bool newKey = false;
bool newByte = false;
bool newWord = false;
bool appMode = true;
char key = '\0';
bool memoryLoaded = false;
//word flashPageAddr = 0xFFFF;  /* Current flash  page address to be written. Tiny85 allowed values are 0 to 0x2000, so 0xFFFF means 'not set' */
word flashPageAddr = 0x0;
word timonelStart = 0xFFFF;   /* Timonel start address, 0xFFFF means 'not set'. Use Timonel 'version' command to get it */

//
// *****************************
// *       Setup Block         *
// *****************************
//
void setup() {
  Serial.begin(9600);   // Init the serial port
              // Init the Wire object for I2C
#if ESP8266
  Wire.begin(0, 2);   // GPIO0 - GPIO2 (ESP-01) // D3 - D4 (NodeMCU)
#else
  Wire.begin();     // Standard pins SDA on D2 and SCL on D1 (NodeMCU)
#endif /* ESP8266 */
              //Wire.begin(D3, D4); // Set SDA on D3 and SCL on D4 (NodeMCU)
  delay(100);       // Wait 100 ms for slave init sequence
              // Search continuouly for slave addresses
  while (slaveAddress == 0) {
    slaveAddress = ScanI2C();
    delay(250);     // Delay 1/4 second before sending I2C commands
  }

  // Run ATtiny85 initialization command
  InitTiny();

  ClrScr();
  Serial.println("Timonel Bootloader and Application I2C Commander Test (v1.1)");
  Serial.println("============================================================");
  TwoStepInit(0);
  Serial.println("");
  ShowMenu();
}
#if !(ESP8266)
void(*resetFunc) (void) = 0;//declare reset function at address 0
#endif /* ESP8266 */

//
// ***************************
// *        Main Loop        *
// ***************************
//
void loop() {
  if (newKey == true) {
    newKey = false;
    Serial.println("");
    Serial.println("");
    switch (key) {
    // *********************************
    // * Test App ||| STDPB1_1 Command *
    // *********************************
    case 'a': case 'A': {
      SetPB1On();
      break;
    }
    // *********************************
    // * Test App ||| STDPB1_0 Command *
    // *********************************
    case 's': case 'S': {
      SetPB1Off();
      break;
    }
    // *********************************
    // * Test App ||| RESETINY Command *
    // *********************************
    case 'x': case 'X': {
      ResetTiny();
      Serial.println("\n  .\n\r . .\n\r. . .\n");
      delay(2000);
#if ESP8266
      ESP.restart();
#else
      resetFunc();
#endif /* ESP8266 */
      break;
    }
    // ******************
    // * Restart Master *
    // ******************
    case 'z': case 'Z': {
      Serial.println("\nResetting ESP8266 ...");
      Serial.println("\n.\n.\n.\n");
#if ESP8266
      ESP.restart();
#else
      resetFunc();
#endif /* ESP8266 */
      break;
    }
    // ********************************
    // * Timonel ::: GETTMNLV Command *
    // ********************************
    case 'v': case 'V': {
      //Serial.println("\nBootloader Cmd >>> Get bootloader version ...");
      GetTimonelVersion();
      break;
    }
    // ********************************
    // * Timonel ::: EXITTMNL Command *
    // ********************************
    case 'r': case 'R': {
      //Serial.println("\nBootloader Cmd >>> Run Application ...");
      RunApplication();
      Serial.println("\n. . .\n\r . .\n\r  .\n");
      delay(2000);
#if ESP8266
      ESP.restart();
#else
      resetFunc();
#endif /* ESP8266 */
      break;
    }
    // ********************************
    // * Timonel ::: DELFLASH Command *
    // ********************************
    case 'e': case 'E': {
      //Serial.println("\nBootloader Cmd >>> Delete app firmware from T85 flash memory ...");
      DeleteFlash();
      TwoStepInit(750);
      break;
    }
    // ********************************
    // * Timonel ::: STPGADDR Command *
    // ********************************
    case 'b': case 'B': {
      byte resetFirstByte = 0;
      byte resetSecondByte = 0;
      Serial.print("Please enter the flash memory page base address: ");
      while (newWord == false) {
        flashPageAddr = ReadWord();
      }
      if (timonelStart > MCUTOTALMEM) {
        Serial.println("\n\n\rWarning: Timonel bootloader start address unknown, please run 'version' command to find it !");
        //newWord = false;
        break;
      }
      if ((flashPageAddr > (timonelStart - 64)) | (flashPageAddr == 0xFFFF)) {
        Serial.print("\n\n\rWarning: The highest flash page addreess available is ");
        Serial.print(timonelStart - 64);
        Serial.print(" (0x");
        Serial.print(timonelStart - 64, HEX);
        Serial.println("), please correct it !!!");
        newWord = false;
        break;
      }
      if (newWord == true) {
        Serial.println("");
        Serial.print("Flash memory page base address: ");
        Serial.println(flashPageAddr);
        Serial.print("Address high byte: ");
        Serial.print((flashPageAddr & 0xFF00) >> 8);
        Serial.print(" (<< 8) + Address low byte: ");
        Serial.print(flashPageAddr & 0xFF);
        SetTmlPageAddr(flashPageAddr);
        newWord = false;
      }
      break;
    }
    // ********************************
    // * Timonel ::: WRITPAGE Command *
    // ********************************
    case 'w': case 'W': {
      WriteFlash();
      break;
    }
    // ********************************
    // * Timonel ::: READFLSH Command *
    // ********************************
    case 'm': case 'M': {
      byte dataSize = 0;  // flash data size requested to ATtiny85
      byte dataIX = 0;  // Requested flash data start position
      DumpFlashMem(MCUTOTALMEM, 8, 32);
      newByte = false;
      break;
    }
    // ******************
    // * ? Help Command *
    // ******************
    case '?': {
      Serial.println("\n\rHelp ...");
      Serial.println("========");
      //ShowHelp();
      break;
    }
    // *******************
    // * Unknown Command *
    // *******************
    default: {
      Serial.print("ESP8266 - Command '");
      Serial.print(key);
      Serial.println("' unknown ...");
      break;
    }
    }
    Serial.println("");
    ShowMenu();
  }
  ReadChar();
}

// Function ScanI2C
byte ScanI2C() {
  //
  // Address 08 to 35: Timonel bootloader
  // Address 36 to 64: Application firmware
  // Each I2C slave must have a unique bootloader address that corresponds
  // to a defined application address, as shown in this table:
  // T: |08|09|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|
  // A: |36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|
  //
  Serial.println("Scanning I2C bus ...");
  byte slaveAddr = 0, scanAddr = 8;
  while (scanAddr < 120) {
    Wire.beginTransmission(scanAddr);
    if (Wire.endTransmission() == 0) {
      if (scanAddr < 36) {
        Serial.print("Timonel Bootloader found at address: ");
        appMode = false;
      }
      else {
        Serial.print("Test App Firmware found at address: ");
        appMode = true;
      }
      Serial.print(scanAddr, DEC);
      Serial.print(" (0x");
      Serial.print(scanAddr, HEX);
      Serial.println(")");
      delay(500);
      slaveAddr = scanAddr;
    }
    scanAddr++;
  }
  return slaveAddr;
}

// Function CalculateCRC (CRC-8)
byte CalculateCRC(byte* block, size_t blockLength) {
  int i;
  byte crc = 0, data = 0;
  for (i = 0; i < blockLength; i++) {
    data = (byte)(block[i] ^ crc); // XOR-in next input byte
    crc = (byte)(crcTable[data]);  // Get current CRC value = remainder 
  }
  return crc;
}

// Function ReadChar
void ReadChar() {
  if (Serial.available() > 0) {
    key = Serial.read();
    newKey = true;
  }
}

// Function ReadByte
byte ReadByte(void) {
  const byte dataLength = 16;
  char serialData[dataLength];  // an array to store the received data  
  static byte ix = 0;
  char rc, endMarker = 0xD;   //standard is: char endMarker = '\n'
  while (Serial.available() > 0 && newByte == false) {
    rc = Serial.read();
    if (rc != endMarker) {
      serialData[ix] = rc;
      Serial.print(serialData[ix]);
      ix++;
      if (ix >= dataLength) {
        ix = dataLength - 1;
      }
    }
    else {
      serialData[ix] = '\0';  // terminate the string
      ix = 0;
      newByte = true;
    }
  }
  if ((atoi(serialData) < 0 || atoi(serialData) > 255) && newByte == true) {
    Serial.println("");
    Serial.print("WARNING! Byte values must be 0 to 255 -> Truncating to ");
    Serial.println((byte)atoi(serialData));
  }
  return((byte)atoi(serialData));
}

// Function ReadWord
word ReadWord(void) {
  const byte dataLength = 16;
  char serialData[dataLength];  // an array to store the received data  
  static byte ix = 0;
  char rc, endMarker = 0xD;   //standard is: char endMarker = '\n'
  while (Serial.available() > 0 && newWord == false) {
    rc = Serial.read();
    if (rc != endMarker) {
      serialData[ix] = rc;
      Serial.print(serialData[ix]);
      ix++;
      if (ix >= dataLength) {
        ix = dataLength - 1;
      }
    }
    else {
      serialData[ix] = '\0';  // terminate the string
      ix = 0;
      newWord = true;
    }
  }
  if ((atoi(serialData) < 0 || atoi(serialData) > MCUTOTALMEM) && newWord == true) {
    for (int i = 0; i < dataLength; i++) {
      serialData[i] = 0;
    }
    Serial.println("");
    Serial.print("WARNING! Word memory positions must be between 0 and ");
    Serial.print(MCUTOTALMEM);
    Serial.print(" -> Changing to ");
    Serial.println((word)atoi(serialData));
  }
  return((word)atoi(serialData));
}

// Function Clear Screen
void ClrScr() {
  Serial.write(27);       // ESC command
  Serial.print("[2J");    // clear screen command
  Serial.write(27);       // ESC command
  Serial.print("[H");     // cursor to home command
}

// Function SetPB1On
void SetPB1On(void) {
  byte cmdTX[1] = { STDPB1_1 };
  byte txSize = sizeof(cmdTX);
  Serial.print("ESP8266 - Sending Opcode >>> ");
  Serial.print(cmdTX[0]);
  Serial.println("(STDPB1_1)");
  // Transmit command
  byte transmitData[1] = { 0 };
  for (int i = 0; i < txSize; i++) {
    transmitData[i] = cmdTX[i];
    Wire.beginTransmission(slaveAddress);
    Wire.write(transmitData[i]);
    Wire.endTransmission();
  }
  // Receive acknowledgement
  blockRXSize = Wire.requestFrom(slaveAddress, (byte)1);
  byte ackRX[1] = { 0 };   // Data received from slave
  for (int i = 0; i < blockRXSize; i++) {
    ackRX[i] = Wire.read();
  }
  if (ackRX[0] == AKDPB1_1) {
    Serial.print("ESP8266 - Command ");
    Serial.print(cmdTX[0]);
    Serial.print(" parsed OK <<< ");
    Serial.println(ackRX[0]);
  }
  else {
    Serial.print("ESP8266 - Error parsing ");
    Serial.print(cmdTX[0]);
    Serial.print(" command! <<< ");
    Serial.println(ackRX[0]);
  }
}

// Function SetPB1Off
void SetPB1Off(void) {
  byte cmdTX[1] = { STDPB1_0 };
  byte txSize = sizeof(cmdTX);
  Serial.print("ESP8266 - Sending Opcode >>> ");
  Serial.print(cmdTX[0]);
  Serial.println("(STDPB1_0)");
  // Transmit command
  byte transmitData[1] = { 0 };
  for (int i = 0; i < txSize; i++) {
    transmitData[i] = cmdTX[i];
    Wire.beginTransmission(slaveAddress);
    Wire.write(transmitData[i]);
    Wire.endTransmission();
  }
  // Receive acknowledgement
  blockRXSize = Wire.requestFrom(slaveAddress, (byte)1);
  byte ackRX[1] = { 0 };   // Data received from slave
  for (int i = 0; i < blockRXSize; i++) {
    ackRX[i] = Wire.read();
  }
  if (ackRX[0] == AKDPB1_0) {
    Serial.print("ESP8266 - Command ");
    Serial.print(cmdTX[0]);
    Serial.print(" parsed OK <<< ");
    Serial.println(ackRX[0]);
  }
  else {
    Serial.print("ESP8266 - Error parsing ");
    Serial.print(cmdTX[0]);
    Serial.print(" command! <<< ");
    Serial.println(ackRX[0]);
  }
}

// Function DumpFlashMem
void DumpFlashMem(word flashSize, byte dataSize, byte valuesPerLine) {
  byte cmdTX[5] = { READFLSH, 0, 0, 0, 0 };
  byte txSize = 5;
  uint8_t checksumErr = 0;
  int v = 1;
  cmdTX[3] = dataSize;
  byte transmitData[1] = { 0 };
  Serial.println("\n\n\r[Timonel] - Dumping Flash Memory ...");
  Serial.println("");

  Serial.print("Addr ");
  Serial.print(0, HEX);
  Serial.print(":    ");

  for (word addr = 0; addr < flashSize; addr += dataSize) {
    //byte dataSize = 0;  // Requested T85 buffer data size
    //byte dataIX = 0;    // Requested T85 buffer data start position
    cmdTX[1] = ((addr & 0xFF00) >> 8);    /* Flash page address high byte */
    cmdTX[2] = (addr & 0xFF);       /* Flash page address low byte */
    cmdTX[4] = (byte)(cmdTX[0] + cmdTX[1] + cmdTX[2] + cmdTX[3]); /* READFLSH Checksum */
    for (int i = 0; i < txSize; i++) {
      transmitData[i] = cmdTX[i];
      Wire.beginTransmission(slaveAddress);
      Wire.write(transmitData[i]);
      Wire.endTransmission();
    }
    // Receive acknowledgement
    blockRXSize = Wire.requestFrom(slaveAddress, (byte)(dataSize + 2));
    byte ackRX[dataSize + 2];   // Data received from slave
    for (int i = 0; i < blockRXSize; i++) {
      ackRX[i] = Wire.read();
    }
    if (ackRX[0] == ACKRDFSH) {
      //Serial.print("ESP8266 - Command ");
      //Serial.print(cmdTX[0]);
      //Serial.print(" parsed OK <<< ");
      //Serial.println(ackRX[0]);
      uint8_t checksum = 0;

      for (uint8_t i = 1; i < (dataSize + 1); i++) {
        if (ackRX[i] < 16) {
          //Serial.print("0x0");
          Serial.print("0");
        }
        //else {
        //  Serial.print("0x");
        //}
        Serial.print(ackRX[i], HEX);      /* Byte values */
        //checksum += (ackRX[i]);
        if (v == valuesPerLine) {
          Serial.println("");
          if ((addr + dataSize) < flashSize) {
            Serial.print("Addr ");
            Serial.print(addr + dataSize, HEX);
            if ((addr + dataSize) < 0x1000) {
              if ((addr + dataSize) < 0x100) {
                Serial.print(":   ");
              }
              else {
                Serial.print(":  ");
              }
            }
            else {
              Serial.print(": ");
            }
          }
          v = 0;
        }
        else {
          Serial.print(" ");
        }
        v++;
        //Serial.println(" |");
        checksum += (uint8_t)ackRX[i];
      }
      //if (checksum + 1 == ackRX[dataSize + 1]) {
      if (checksum == ackRX[dataSize + 1]) {
        //Serial.print("   >>> Checksum OK! <<<   ");
        //Serial.println(checksum);
      }
      else {
        Serial.print("\n\r   ### Checksum ERROR! ###   ");
        Serial.println(checksum);
        //Serial.print(checksum + 1);
        //Serial.print(" <-- calculated, received --> ");
        //Serial.println(ackRX[dataSize + 1]);
        if (checksumErr++ == MAXCKSUMERRORS) {
          Serial.println("[Timonel] - Too many Checksum ERRORS, aborting! ");
          delay(1000);
          exit(1);
        }
      }
    }
    else {
      Serial.print("[Timonel] - DumpFlashMem Error parsing ");
      Serial.print(cmdTX[0]);
      Serial.print(" command! <<< ");
      Serial.println(ackRX[0]);
    }
    delay(100);
  }
}

// Function WritePageBuff
int WritePageBuff(uint8_t dataArray[]) {
  const byte txSize = TXDATASIZE + 2;
  byte cmdTX[txSize] = { 0 };
  int commErrors = 0;         /* I2C communication error counter */
  uint8_t checksum = 0;
  //Serial.println("");
  cmdTX[0] = WRITPAGE;
  for (int b = 1; b < txSize - 1; b++) {
    cmdTX[b] = dataArray[b - 1];
    checksum += (byte)dataArray[b - 1];
  }
  cmdTX[txSize - 1] = checksum;
  //Serial.print("[Timonel] Writting data to Attiny85 memory page buffer >>> ");
  //Serial.print(cmdTX[0]);
  //Serial.println("(WRITBUFF)");
  // Transmit command
  byte transmitData[txSize] = { 0 };
  //Serial.print("[Timonel] - Sending data >>> ");
  for (int i = 0; i < txSize; i++) {
    //if (i > 0) {
    //  if (i < txSize - 1) {
    //    Serial.print("0x");
    //    Serial.print(cmdTX[i], HEX);
    //    Serial.print(" ");WritePageBuff
    //  }
    //  else {
    //    Serial.print("\n\r[Timonel] - Sending CRC >>> ");
    //    Serial.println(cmdTX[i]);
    //  }
    //}
    transmitData[i] = cmdTX[i];
    Wire.beginTransmission(slaveAddress);
    Wire.write(transmitData[i]);
    Wire.endTransmission();
  }
  // Receive acknowledgement
  blockRXSize = Wire.requestFrom(slaveAddress, (byte)2);
  byte ackRX[2] = { 0 };   // Data received from slave
  for (int i = 0; i < blockRXSize; i++) {
    ackRX[i] = Wire.read();
  }
  if (ackRX[0] == ACKWTPAG) {
    //Serial.print("[Timonel] - Command ");
    //Serial.print(cmdTX[0]);
    //Serial.print(" parsed OK <<< ");
    //Serial.println(ackRX[0]);
    if (ackRX[1] == checksum) {
      //Serial.print("[Timonel] - Data parsed OK by slave <<< Checksum = 0x");
      //Serial.println(ackRX[1], HEX);
      //Serial.println("");
    }
    else {
      Serial.print("[Timonel] - Data parsed with {{{ERROR}}} <<< Checksum = 0x");
      Serial.println(ackRX[1], HEX);
      //Serial.println("");
      if (commErrors++ > 0) {         /* Checksum error detected ... */
        Serial.println("\n\r[Timonel] - WritePageBuff Checksum Errors, Aborting ...");
        exit(commErrors);
      }
    }
  }
  else {
    Serial.print("[Timonel] - Error parsing ");
    Serial.print(cmdTX[0]);
    Serial.print(" command! <<< ");
    Serial.println(ackRX[0]);
    Serial.println("");
    if (commErrors++ > 0) {         /* Opcode error detected ... */
      Serial.println("\n\r[Timonel] - WritePageBuff Opcode Reply Errors, Aborting ...");
      exit(commErrors);
    }
  }
  return(commErrors);
}

//Function ResetTiny
void ResetTiny(void) {
  Serial.println("Sending ATtiny85 Reset Command ...");
  byte cmdTX[1] = { RESETINY };
  byte txSize = sizeof(cmdTX);
  Serial.print("ESP8266 - Sending Opcode >>> ");
  Serial.print(cmdTX[0]);
  Serial.println("(RESETINY)");
  // Transmit command
  byte transmitData[1] = { 0 };
  for (int i = 0; i < txSize; i++) {
    transmitData[i] = cmdTX[i];
    Wire.beginTransmission(slaveAddress);
    Wire.write(transmitData[i]);
    Wire.endTransmission();
  }
  // Receive acknowledgement
  blockRXSize = Wire.requestFrom(slaveAddress, (byte)1);
  byte ackRX[1] = { 0 };   // Data received from slave
  for (int i = 0; i < blockRXSize; i++) {
    ackRX[i] = Wire.read();
  }
  if (ackRX[0] == ACKRESTY) {
    Serial.print("ESP8266 - Command ");
    Serial.print(cmdTX[0]);
    Serial.print(" parsed OK <<< ");
    Serial.println(ackRX[0]);
  }
  else {
    Serial.print("ESP8266 - Error parsing ");
    Serial.print(cmdTX[0]);
    Serial.print(" command! <<< ");
    Serial.println(ackRX[0]);
  }
}

// Function InitTiny
void InitTiny(void) {
  byte cmdTX[1] = { INITTINY };
  Wire.beginTransmission(slaveAddress);
  Wire.write(cmdTX[0]);
  Wire.endTransmission();
  blockRXSize = Wire.requestFrom(slaveAddress, (byte)1);
  blockRXSize = 0;
}

// Function TwoStepInit
void TwoStepInit(word time) {
  delay(time);
  InitTiny();       /* Two-step Tiny85 initialization: STEP 1 */
  GetTimonelVersion();  /* Two-step Tiny85 initialization: STEP 2 */
}

// Function GetTimonelVersion
void GetTimonelVersion(void) {
  byte cmdTX[1] = { GETTMNLV };
  byte txSize = sizeof(cmdTX);
  Serial.print("\nGet Timonel Version >>> ");
  Serial.print(cmdTX[0]);
  Serial.println("(GETTMNLV)");
  // Transmit command
  byte transmitData[1] = { 0 };
  for (int i = 0; i < txSize; i++) {
    transmitData[i] = cmdTX[i];
    Wire.beginTransmission(slaveAddress);
    Wire.write(transmitData[i]);
    Wire.endTransmission();
  }
  // Receive acknowledgement
  blockRXSize = Wire.requestFrom(slaveAddress, (byte)9);
  byte ackRX[9] = { 0 };   // Data received from slave
  for (int i = 0; i < blockRXSize; i++) {
    ackRX[i] = Wire.read();
  }
  if (ackRX[0] == ACKTMNLV) {
    timonelStart = (ackRX[5] << 8) + ackRX[6];
    word trampolineJump = (~(((ackRX[7] << 8) | ackRX[8]) & 0xFFF));
    trampolineJump = ((((timonelStart >> 1) - ++trampolineJump) & 0xFFF) << 1);
    //Serial.print("[Timonel] - Command ");
    //Serial.print(cmdTX[0]);
    //Serial.print(" parsed OK <<< ");
    //Serial.println(ackRX[0]);
    Serial.println(" ____________________________________");
    Serial.println("| ");
    if (ackRX[1] == 84) {
      Serial.print("| Timonel Bootloader v");
    }
    Serial.print(ackRX[2]);
    Serial.print(".");
    Serial.print(ackRX[3]);
    switch (ackRX[2]) {
      case 0: {
        Serial.println(" Pre-release ");
        break;
      }
      case 1: {
        Serial.println(" \"Sandra\" ");
        break;
      }
      default: {
        Serial.println(" Unknown ");
        break;
      }
    }
    Serial.println("| ================================");
    Serial.print("| Bootloader address: 0x");
    Serial.print(timonelStart, HEX);
    Serial.println(" ");
    Serial.print("|  Application start: ");
    Serial.print(ackRX[8], HEX);
    Serial.print(ackRX[7], HEX);
    if ((ackRX[8] == 0xFF) && (ackRX[7] == 0xFF)) {
      Serial.print(" (Not Set");
    }
    else {
      Serial.print(" (0x");
      Serial.print(trampolineJump, HEX);
    }
    Serial.println(") ");
    //if (ackRX[9] == 0) {
    //  Serial.println("|       Flash Memory: ** Clear **");
    //  memoryLoaded = false;
    //}
    //else {
    //  Serial.println("|       Flash Memory: >> Loaded <<");
    //  memoryLoaded = true;
    //}
    Serial.print("|      Features Code: ");
    Serial.print(ackRX[4]);
    Serial.println(" ");
    Serial.println(" ____________________________________");
    Serial.println("");
  }
  else {
    Serial.print("[Timonel] - Error parsing ");
    Serial.print(cmdTX[0]);
    Serial.print(" command! <<< ");
    Serial.println(ackRX[0]);
  }
}

// Function RunApplication
void RunApplication(void) {
  byte cmdTX[1] = { EXITTMNL };
  byte txSize = sizeof(cmdTX);
  Serial.print("\n[Timonel] Exit bootloader & run application >>> ");
  //Serial.print("ESP8266 - Sending Opcode >>> ");
  Serial.print(cmdTX[0]);
  Serial.println("(EXITTMNL)");
  // Transmit command
  byte transmitData[1] = { 0 };
  for (int i = 0; i < txSize; i++) {
    transmitData[i] = cmdTX[i];
    Wire.beginTransmission(slaveAddress);
    Wire.write(transmitData[i]);
    Wire.endTransmission();
  }
  // Receive acknowledgement
  blockRXSize = Wire.requestFrom(slaveAddress, (byte)1);
  byte ackRX[1] = { 0 };   // Data received from slave
  for (int i = 0; i < blockRXSize; i++) {
    ackRX[i] = Wire.read();
  }
  if (ackRX[0] == ACKEXITT) {
    Serial.print("[Timonel] - Command ");
    Serial.print(cmdTX[0]);
    Serial.print(" parsed OK <<< ");
    Serial.println(ackRX[0]);
  }
  else {
    Serial.print("[Timonel] - Error parsing ");
    Serial.print(cmdTX[0]);
    Serial.print(" command! <<< ");
    Serial.println(ackRX[0]);
  }
}

// Function DeleteFlash
void DeleteFlash(void) {
  byte cmdTX[1] = { DELFLASH };
  byte txSize = sizeof(cmdTX);
  Serial.print("\n[Timonel] Delete Flash Memory >>> ");
  //Serial.print("ESP8266 - Sending Opcode >>> ");
  Serial.print(cmdTX[0]);
  Serial.println("(DELFLASH)");
  // Transmit command
  byte transmitData[1] = { 0 };
  for (int i = 0; i < txSize; i++) {
    transmitData[i] = cmdTX[i];
    Wire.beginTransmission(slaveAddress);
    Wire.write(transmitData[i]);
    Wire.endTransmission();
  }
  // Receive acknowledgement
  blockRXSize = Wire.requestFrom(slaveAddress, (byte)1);
  byte ackRX[1] = { 0 };   // Data received from slave
  for (int i = 0; i < blockRXSize; i++) {
    ackRX[i] = Wire.read();
  }
  if (ackRX[0] == ACKDELFL) {
    Serial.print("[Timonel] - Command ");
    Serial.print(cmdTX[0]);
    Serial.print(" parsed OK <<< ");
    Serial.println(ackRX[0]);
  }
  else {
    Serial.print("[Timonel] - Error parsing ");
    Serial.print(cmdTX[0]);
    Serial.print(" command! <<< ");
    Serial.println(ackRX[0]);
  }
}

// Function SetTmlPageAddr
void SetTmlPageAddr(word pageAddr) {
  byte cmdTX[4] = { STPGADDR, 0, 0, 0 };
  byte txSize = 4;
  Serial.println("");
  cmdTX[1] = ((pageAddr & 0xFF00) >> 8);    /* Flash page address high byte */
  cmdTX[2] = (pageAddr & 0xFF);       /* Flash page address low byte */
  Serial.print("\n[Timonel] Setting flash page address on Attiny85 >>> ");
  Serial.print(cmdTX[0]);
  Serial.println("(STPGADDR)");
  cmdTX[3] = CalculateCRC(cmdTX, 2);
  // Transmit command
  byte transmitData[4] = { 0 };
  for (int i = 0; i < txSize; i++) {
    if (i > 0) {
      if (i < txSize - 1) {
        Serial.print("[Timonel] - Sending Operand >>> ");
        Serial.println(cmdTX[i]);
      }
      else {
        Serial.print("[Timonel] - Sending CRC >>> ");
        Serial.println(cmdTX[i]);
      }
    }
    transmitData[i] = cmdTX[i];
    Wire.beginTransmission(slaveAddress);
    Wire.write(transmitData[i]);
    Wire.endTransmission();
  }
  // Receive acknowledgement
  blockRXSize = Wire.requestFrom(slaveAddress, (byte)2);
  byte ackRX[2] = { 0 };   // Data received from slave
  for (int i = 0; i < blockRXSize; i++) {
    ackRX[i] = Wire.read();
  }
  if (ackRX[0] == AKPGADDR) {
    Serial.print("[Timonel] - Command ");
    Serial.print(cmdTX[0]);
    Serial.print(" parsed OK <<< ");
    Serial.println(ackRX[0]);
    if (ackRX[1] == (byte)(cmdTX[1] + cmdTX[2])) {
      Serial.print("[Timonel] - Operands ");
      Serial.print(cmdTX[1]);
      Serial.print(" and ");
      Serial.print(cmdTX[2]);
      Serial.print(" parsed OK by slave <<< ATtiny85 Flash Page Address Check = ");
      Serial.println(ackRX[1]);
    }
    else {
      Serial.print("[Timonel] - Operand ");
      Serial.print(cmdTX[1]);
      Serial.print(" parsed with {{{ERROR}}} <<< ATtiny85 Flash Page Address Check = ");
      Serial.println(ackRX[1]);
    }

  }
  else {
    Serial.print("[Timonel] - Error parsing ");
    Serial.print(cmdTX[0]);
    Serial.print(" command! <<< ");
    Serial.println(ackRX[0]);
  }
}

// Function WriteFlash
int WriteFlash(void) {
  int packet = 0;               /* Byte counter to be sent in a single I2C data packet */
  int padding = 0;              /* Amount of padding bytes to match the page size */
  int pageEnd = 0;              /* Byte counter to detect the end of flash mem page */
  int pageCount = 1;
  int wrtErrors = 0;
  uint8_t dataPacket[TXDATASIZE] = { 0xFF };
  int payloadSize = sizeof(payload);
  if ((payloadSize % FLASHPGSIZE) != 0) {   /* If the payload to be sent is smaller than flash page size, resize it to match */
    padding = ((((int)(payloadSize / FLASHPGSIZE) + 1) * FLASHPGSIZE) - payloadSize);
    payloadSize += padding;
  }
  Serial.println("\nWriting payload to flash ...\n\r");
  //if (flashPageAddr == 0xFFFF) {
  //  Serial.println("Warning: Flash page start address no set, please use 'b' command to set it ...\n\r");
  //  return(1);
  //}
  //Serial.print("::::::::::::::::::::::::::::::::::::::: Page ");
  //Serial.print(pageCount);
  //Serial.print(" - Address 0x");
  //Serial.println(flashPageAddr, HEX);
  for (int i = 0; i < payloadSize; i++) {
    if (i < (payloadSize - padding)) {
      dataPacket[packet] = payload[i];    /* If there are data to fill the page, use it ... */
    }
    else {
      dataPacket[packet] = 0xff;        /* If there are no more data, complete the page with padding (0xff) */
    }
    if (packet++ == (TXDATASIZE - 1)) {     /* When a data packet is completed to be sent ... */
      for (int b = 0; b < TXDATASIZE; b++) {
        //Serial.print("0x");
        //if (dataPacket[b] < 0x10) {
        //  Serial.print("0");
        //}
        //Serial.print(dataPacket[b], HEX);
        //Serial.print(" ");
        Serial.print(".");
      }
      wrtErrors += WritePageBuff(dataPacket); /* Send data to T85 through I2C */
      packet = 0;
      delay(10);                /* ###### DELAY BETWEEN PACKETS SENT TO PAGE ###### */
    }
    if (wrtErrors > 0) {
      //Serial.println("\n\r==== WriteFlash: There were transmission errors, aborting ...");
      //DeleteFlash();
      TwoStepInit(2000);
#if ESP8266
      ESP.restart();
#else
      resetFunc();
#endif /* ESP8266 */
      return(wrtErrors);
    }
    if (pageEnd++ == (FLASHPGSIZE - 1)) {   /* When a page end is detected ... */

      Serial.print(pageCount++);
      //DumpPageBuff(FLASHPGSIZE, TXDATASIZE, TXDATASIZE);
      delay(100);               /* ###### DELAY BETWEEN PAGE WRITINGS ... ###### */

      if (i < (payloadSize - 1)) {
        //Serial.print("::::::::::::::::::::::::::::::::::::::: Page ");
        //Serial.print(++pageCount);
        //Serial.print(" - Address 0x");
        //Serial.println(((flashPageAddr + 1 + i) & 0xFFFF), HEX);
        pageEnd = 0;
      }
    }
  }
  if (wrtErrors == 0) {
    Serial.println("\n\n\r==== WriteFlash: Firmware was successfully transferred to T85, please select 'run app' command to start it ...");
  }
  else {
    Serial.print("\n\n\r==== WriteFlash: Communication errors detected during firmware transfer, please retry !!! ErrCnt: ");
    Serial.print(wrtErrors);
    Serial.println(" ===");
    //DeleteFlash();
    TwoStepInit(2000);
#if ESP8266
    ESP.restart();
#else
    resetFunc();
#endif /* ESP8266 */
  }
  return(wrtErrors);
}

//Function ShowMenu
void ShowMenu(void) {
  if (appMode == true) {
    Serial.print("Application command ('a', 's', 'z' reboot, 'x' reset T85, '?' help): ");
  }
  else {
    Serial.print("Timonel booloader ('v' version, 'r' run app, 'e' erase flash, 'w' write flash, 'm' mem dump): ");
  }
}

//Function ShowTrampoline
void ShowTrampoline(void) {
#define TIMONEL_START 0x1A40
#define LSB 0x0E
#define MSB 0xC0
  Serial.print("\nTIMONEL START = 0x");
  Serial.println(TIMONEL_START, HEX);
  Serial.print("LSB = 0x");
  Serial.print(LSB, HEX);
  Serial.print(" ||| MSB = 0x");
  Serial.println(MSB, HEX);
  word jumpOffset = ((MSB << 8) | LSB);
  Serial.print("QQ = 0x");
  Serial.println(jumpOffset, HEX);
  jumpOffset = (((~((TIMONEL_START >> 1) - (++jumpOffset & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
  Serial.print("JUMP ADDRESS = 0x");
  Serial.println(jumpOffset, HEX);
}
