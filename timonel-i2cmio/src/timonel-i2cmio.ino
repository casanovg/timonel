// ********************************************************
// *  Timonel I2C Master                                  *
// *  ==================                                  *
// *  I2C Master for Bootloader Tests                     *
// *  ..................................................  *
// *  Author: Gustavo Casanova                            *
// *  ..................................................  *
// *  Firmware Version: 1.2 | MCU: ESP8266                *
// *  2018-11-27 gustavo.casanova@nicebots.com            *
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

//#define ESP8266      true  /* True = ESP8266, False = Arduino */

#include <Wire.h>
#include "nb-i2c-cmd.h"
#include "nb-i2c-crc.h"
#if ESP8266
#include <pgmspace.h>
#endif /* ESP8266 */
#include "Payloads/payload.h"

#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <WiFiManager.h>

// Timonel bootloader
#define MCUTOTALMEM   8192  /* Slave MCU total flash memory*/
#define MAXCKSUMERRORS  100   /* Max number of checksum errors allowed in bootloader comms */
#define TXDATASIZE    8   /* TX data size for WRITBUFF command */
#define FLASHPGSIZE   64    /* Tiny85 flash page buffer size */
#define DATATYPEBYTE  1   /* Buffer data type "Byte" */
#define USE_SERIAL Serial

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

//const char* ssid = "Nicebots.com";    // Set your router SSID
//const char* password = "R2-D2 C-3P0"; // Set your router password

ESP8266WiFiMulti WiFiMulti;						// Wifi interface

//
// *****************************
// *       Setup Block         *
// *****************************
//
void setup() {

	USE_SERIAL.begin(9600);   // Init the serial port
	ClrScr();

	ShowHeader();

	for (uint8_t t = 4; t > 0; t--) {
		USE_SERIAL.printf_P("Wait %d ...\n\r", t);
		USE_SERIAL.flush();
		delay(1000);
	}

	WiFiManager wm;

  bool connectionStatus;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  connectionStatus = wm.autoConnect("AutoConnectAP","password"); // password protected ap

  if(!connectionStatus) {
    USE_SERIAL.println(F("Failed to connect, rebooting"));
    delay(3000);
    ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi
		USE_SERIAL.println(F("Connected to access point!"));

		t_httpUpdate_return ret = ESPhttpUpdate.update(F("http://fw.nicebots.com/bin/hurlingham.bin"));
		//t_httpUpdate_return ret = ESPhttpUpdate.update(F("http://fw.nicebots.com/update.php"));
    //t_httpUpdate_return  ret = ESPhttpUpdate.update("https://server/file.bin", "", "fingerprint");
    switch (ret) {
    	case HTTP_UPDATE_FAILED:
      	USE_SERIAL.printf_P("HTTP_UPDATE_FAILED Error (%d): %s\n\r", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;
      case HTTP_UPDATE_NO_UPDATES:
      	USE_SERIAL.println(F("HTTP_UPDATE_NO_UPDATES\n\r"));
        break;

      case HTTP_UPDATE_OK:
        USE_SERIAL.println(F("HTTP_UPDATE_OK\n\r"));
      	break;
      }

      //USE_SERIAL.println(F("\n\rNB setup and update finished, starting loop code ...\n\r"));
      //USE_SERIAL.printf_P("Led blink delay: 0x%04X\n\n\r", blinkDly);

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
			TwoStepInit(0);

			USE_SERIAL.println(F(".\n\n\r"));
			delay(2000);

			ShowHeader();
			ShowMenu();
  }
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
   USE_SERIAL.println();
   USE_SERIAL.println();
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
     USE_SERIAL.println(F("\n  .\n\r . .\n\r. . .\n"));
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
     USE_SERIAL.println(F("\nResetting ESP8266 ..."));
     USE_SERIAL.println(F("\n.\n.\n.\n"));
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
     //USE_SERIAL.println(F("\nBootloader Cmd >>> Get bootloader version ..."));
     GetTimonelVersion();
     break;
   }
   // ********************************
   // * Timonel ::: EXITTMNL Command *
   // ********************************
   case 'r': case 'R': {
     //USE_SERIAL.println(F("\nBootloader Cmd >>> Run Application ..."));
     RunApplication();
     USE_SERIAL.println(F("\n. . .\n\r . .\n\r  .\n"));
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
     //USE_SERIAL.println(F("\nBootloader Cmd >>> Delete app firmware from T85 flash memory ..."));
     DeleteFlash();
     TwoStepInit(750);
     break;
   }
   // ********************************
   // * Timonel ::: STPGADDR Command *
   // ********************************
   case 'b': case 'B': {
     //byte resetFirstByte = 0;
     //byte resetSecondByte = 0;
     USE_SERIAL.print(F("Please enter the flash memory page base address: "));
     while (newWord == false) {
       flashPageAddr = ReadWord();
     }
     if (timonelStart > MCUTOTALMEM) {
       USE_SERIAL.println(F("\n\n\rWarning: Timonel bootloader start address unknown, please run 'version' command to find it !"));
       //newWord = false;
       break;
     }
     if ((flashPageAddr > (timonelStart - 64)) | (flashPageAddr == 0xFFFF)) {

       USE_SERIAL.printf_P("\n\n\rWarning: The highest flash page addreess available is %04X, please correct it !!!\n\r", timonelStart - 64);

       // USE_SERIAL.print(F("\n\n\rWarning: The highest flash page addreess available is "));
       // USE_SERIAL.print(timonelStart - 64);
       // USE_SERIAL.print(F(" (0x"));
       // USE_SERIAL.print(timonelStart - 64, HEX);
       // USE_SERIAL.println(F("), please correct it !!!"));
       newWord = false;
       break;
     }
     if (newWord == true) {

			 USE_SERIAL.printf_P("\n\rFlash memory page base address: %04X\n\r", flashPageAddr);
			 USE_SERIAL.printf_P("\n\rAddress high byte: %02X  (<< 8) + Address low byte: %02X\n\r", (flashPageAddr & 0xFF00) >> 8, flashPageAddr & 0xFF);

       // USE_SERIAL.println();
       // USE_SERIAL.print(F("Flash memory page base address: "));
       // USE_SERIAL.println(flashPageAddr);
       // USE_SERIAL.print(F("Address high byte: "));
       // USE_SERIAL.print((flashPageAddr & 0xFF00) >> 8);
       // USE_SERIAL.print(F(" (<< 8) + Address low byte: "));
       // USE_SERIAL.print(flashPageAddr & 0xFF);
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
     byte dataSize = 8;  // flash data size requested to ATtiny85
     byte valuesPerLine = 32;  // Requested flash data start position
     DumpFlashMem(MCUTOTALMEM, dataSize, valuesPerLine);
     newByte = false;
     break;
   }
   // ******************
   // * ? Help Command *
   // ******************
   case '?': {
     USE_SERIAL.println(F("\n\rHelp ...\n\r========\n\r"));
     //USE_SERIAL.println(F("========"));
     //ShowHelp();
     break;
   }
   // *******************
   // * Unknown Command *
   // *******************
   default: {

		 USE_SERIAL.printf_P("ESP8266 - Command '%c' unknown ...\n\r", key);

     // USE_SERIAL.print(F("ESP8266 - Command '"));
     // USE_SERIAL.print(key);
     // USE_SERIAL.println(F("' unknown ..."));
     break;
   }
   }
   USE_SERIAL.println(F(""));
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
  USE_SERIAL.println(F("Scanning I2C bus ..."));
  byte slaveAddr = 0, scanAddr = 8;
  while (scanAddr < 120) {
    Wire.beginTransmission(scanAddr);
    if (Wire.endTransmission() == 0) {
      if (scanAddr < 36) {
				USE_SERIAL.printf_P("Timonel Bootloader found at address: %d (0X%02X)\n\r", scanAddr, scanAddr);
        //USE_SERIAL.print(F("Timonel Bootloader found at address: "));
        appMode = false;
      }
      else {
				USE_SERIAL.printf_P("Test App Firmware found at address: %d (0X%02X)\n\r", scanAddr, scanAddr);
        //USE_SERIAL.print(F("Test App Firmware found at address: "));
        appMode = true;
      }
      // USE_SERIAL.print(scanAddr, DEC);
      // USE_SERIAL.print(F(" (0x"));
      // USE_SERIAL.print(scanAddr, HEX);
      // USE_SERIAL.println(F(")"));
      delay(500);
      slaveAddr = scanAddr;
    }
    scanAddr++;
  }
  return slaveAddr;
}

// Function CalculateCRC (CRC-8)
byte CalculateCRC(byte* block, size_t blockLength) {
 unsigned int i;
 byte crc = 0, data = 0;
 for (i = 0; i < blockLength; i++) {
   data = (byte)(block[i] ^ crc); // XOR-in next input byte
   crc = (byte)(crcTable[data]);  // Get current CRC value = remainder
 }
 return crc;
}

// Function ReadChar
void ReadChar() {
  if (USE_SERIAL.available() > 0) {
    key = USE_SERIAL.read();
    newKey = true;
  }
}

// Function ReadByte
byte ReadByte(void) {
  const byte dataLength = 16;
  char serialData[dataLength];  // an array to store the received data
  static byte ix = 0;
  char rc, endMarker = 0xD;   //standard is: char endMarker = '\n'
  while (USE_SERIAL.available() > 0 && newByte == false) {
    rc = USE_SERIAL.read();
    if (rc != endMarker) {
      serialData[ix] = rc;
      USE_SERIAL.print(serialData[ix]);
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
    //USE_SERIAL.println();
		USE_SERIAL.printf_P("WARNING! Byte values must be 0 to 255 -> Truncating to %d\n\r", (byte)atoi(serialData));
    //USE_SERIAL.print(F("WARNING! Byte values must be 0 to 255 -> Truncating to "));
    //USE_SERIAL.println((byte)atoi(serialData));
  }
  return((byte)atoi(serialData));
}

// Function ReadWord
word ReadWord(void) {
  const byte dataLength = 16;
  char serialData[dataLength];  // an array to store the received data
  static byte ix = 0;
  char rc, endMarker = 0xD;   //standard is: char endMarker = '\n'
  while (USE_SERIAL.available() > 0 && newWord == false) {
    rc = USE_SERIAL.read();
    if (rc != endMarker) {
      serialData[ix] = rc;
      USE_SERIAL.print(serialData[ix]);
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

		USE_SERIAL.printf_P("\n\rWARNING! Word memory positions must be between 0 and %d -> Changing to %d\n\r", MCUTOTALMEM, (word)atoi(serialData));

    // USE_SERIAL.println();
    // USE_SERIAL.print(F("WARNING! Word memory positions must be between 0 and "));
    // USE_SERIAL.print(MCUTOTALMEM);
    // USE_SERIAL.print(F(" -> Changing to "));
    // USE_SERIAL.println((word)atoi(serialData));
  }
  return((word)atoi(serialData));
}

// Function Clear Screen
void ClrScr() {
  USE_SERIAL.write(27);       // ESC command
  USE_SERIAL.print(F("[2J"));    // clear screen command
  USE_SERIAL.write(27);       // ESC command
  USE_SERIAL.print(F("[H"));     // cursor to home command
}

// Function SetPB1On
void SetPB1On(void) {
  byte cmdTX[1] = { STDPB1_1 };
  byte txSize = sizeof(cmdTX);

	USE_SERIAL.printf_P("ESP8266 - Sending Opcode >>> %d (STDPB1_1)\n\r", cmdTX[0]);

  // USE_SERIAL.print(F("ESP8266 - Sending Opcode >>> "));
  // USE_SERIAL.print(cmdTX[0]);
  // USE_SERIAL.println(F("(STDPB1_1)"));
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

		USE_SERIAL.printf_P("ESP8266 - Command %d parsed OK <<< %d\n\r", cmdTX[0], ackRX[0]);

    // USE_SERIAL.print(F("ESP8266 - Command "));
    // USE_SERIAL.print(cmdTX[0]);
    // USE_SERIAL.print(F(" parsed OK <<< "));
    // USE_SERIAL.println(ackRX[0]);
  }
  else {

		USE_SERIAL.printf_P("ESP8266 - Error parsing %d command! <<< %d\n\r", cmdTX[0], ackRX[0]);

    // USE_SERIAL.print(F("ESP8266 - Error parsing "));
    // USE_SERIAL.print(cmdTX[0]);
    // USE_SERIAL.print(F(" command! <<< "));
    // USE_SERIAL.println(ackRX[0]);
  }
}

// Function SetPB1Off
void SetPB1Off(void) {
	byte cmdTX[1] = { STDPB1_0 };
  byte txSize = sizeof(cmdTX);

	USE_SERIAL.printf_P("ESP8266 - Sending Opcode >>> %d (STDPB1_0)\n\r", cmdTX[0]);

  // USE_SERIAL.print(F("ESP8266 - Sending Opcode >>> "));
  // USE_SERIAL.print(cmdTX[0]);
  // USE_SERIAL.println(F("(STDPB1_0)"));
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

		USE_SERIAL.printf_P("ESP8266 - Command %d parsed OK <<< %d\n\r", cmdTX[0], ackRX[0]);

    // USE_SERIAL.print(F("ESP8266 - Command "));
    // USE_SERIAL.print(cmdTX[0]);
    // USE_SERIAL.print(F(" parsed OK <<< "));
    // USE_SERIAL.println(ackRX[0]);
  }
  else {

		USE_SERIAL.printf_P("ESP8266 - Error parsing %d command! <<< %d\n\r", cmdTX[0], ackRX[0]);

    // USE_SERIAL.print(F("ESP8266 - Error parsing "));
    // USE_SERIAL.print(cmdTX[0]);
    // USE_SERIAL.print(F(" command! <<< "));
    // USE_SERIAL.println(ackRX[0]);
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
  USE_SERIAL.println(F("\n\n\r[Timonel] - Dumping Flash Memory ..."));
  USE_SERIAL.println();

	USE_SERIAL.printf_P("Addr 0:    ");

  // USE_SERIAL.print(F("Addr "));
  // USE_SERIAL.print(0, HEX);
  // USE_SERIAL.print(F(":    "));

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
      //USE_SERIAL.print(F("ESP8266 - Command "));
      //USE_SERIAL.print(cmdTX[0]);
      //USE_SERIAL.print(F(" parsed OK <<< "));
      //USE_SERIAL.println(ackRX[0]);
      uint8_t checksum = 0;

      for (uint8_t i = 1; i < (dataSize + 1); i++) {
        if (ackRX[i] < 16) {
          //USE_SERIAL.print(F("0x0");
          USE_SERIAL.print(F("0"));
        }
        //else {
        //  USE_SERIAL.print(F("0x"));
        //}
        USE_SERIAL.print(ackRX[i], HEX);      /* Byte values */
        //checksum += (ackRX[i]);
        if (v == valuesPerLine) {
          USE_SERIAL.println();
          if ((addr + dataSize) < flashSize) {

						USE_SERIAL.printf_P("Addr %04X\n\r", addr + dataSize);

            // USE_SERIAL.print(F("Addr "));
            // USE_SERIAL.print(addr + dataSize, HEX);
            if ((addr + dataSize) < 0x1000) {
              if ((addr + dataSize) < 0x100) {
                USE_SERIAL.print(F(":   "));
              }
              else {
                USE_SERIAL.print(F(":  "));
              }
            }
            else {
              USE_SERIAL.print(F(": "));
            }
          }
          v = 0;
        }
        else {
          USE_SERIAL.print(F(" "));
        }
        v++;
        //USE_SERIAL.println(F(" |"));
        checksum += (uint8_t)ackRX[i];
      }
      //if (checksum + 1 == ackRX[dataSize + 1]) {
      if (checksum == ackRX[dataSize + 1]) {
        //USE_SERIAL.print(F("   >>> Checksum OK! <<<   "));
        //USE_SERIAL.println(checksum);
      }
      else {

				USE_SERIAL.printf_P("\n\r   ### Checksum ERROR! ###   %d\n\r", checksum);

        // USE_SERIAL.print(F("\n\r   ### Checksum ERROR! ###   "));
        // USE_SERIAL.println(checksum);
        //USE_SERIAL.print(checksum + 1);
        //USE_SERIAL.print(F(" <-- calculated, received --> "));
        //USE_SERIAL.println(ackRX[dataSize + 1]);
        if (checksumErr++ == MAXCKSUMERRORS) {
          USE_SERIAL.println(F("[Timonel] - Too many Checksum ERRORS, aborting! "));
          delay(1000);
          exit(1);
        }
      }
    }
    else {

			USE_SERIAL.printf_P("[Timonel] - DumpFlashMem Error parsing %d command! <<< %d\n\r", cmdTX[0], ackRX[0]);

      // USE_SERIAL.print(F("[Timonel] - DumpFlashMem Error parsing "));
      // USE_SERIAL.print(cmdTX[0]);
      // USE_SERIAL.print(F(" command! <<< "));
      // USE_SERIAL.println(ackRX[0]);
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
  //USE_SERIAL.println();
  cmdTX[0] = WRITPAGE;
  for (int b = 1; b < txSize - 1; b++) {
    cmdTX[b] = dataArray[b - 1];
    checksum += (byte)dataArray[b - 1];
  }
  cmdTX[txSize - 1] = checksum;
  //USE_SERIAL.print(F("[Timonel] Writting data to Attiny85 memory page buffer >>> "));
  //USE_SERIAL.print(cmdTX[0]);
  //USE_SERIAL.println(F("(WRITBUFF)"));
  // Transmit command
  byte transmitData[txSize] = { 0 };
  //USE_SERIAL.print(F("[Timonel] - Sending data >>> "));
  for (int i = 0; i < txSize; i++) {
    //if (i > 0) {
    //  if (i < txSize - 1) {
    //    USE_SERIAL.print(F("0x"));
    //    USE_SERIAL.print(cmdTX[i], HEX);
    //    USE_SERIAL.print(); //WritePageBuff
    //  }
    //  else {
    //    USE_SERIAL.print(F("\n\r[Timonel] - Sending CRC >>> "));
    //    USE_SERIAL.println(cmdTX[i]);
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
    //USE_SERIAL.print(F("[Timonel] - Command "));
    //USE_SERIAL.print(cmdTX[0]);
    //USE_SERIAL.print(F(" parsed OK <<< "));
    //USE_SERIAL.println(ackRX[0]);
    if (ackRX[1] == checksum) {
      //USE_SERIAL.print(F("[Timonel] - Data parsed OK by slave <<< Checksum = 0x"));
      //USE_SERIAL.println(ackRX[1], HEX);
      //USE_SERIAL.println();
    }
    else {

			USE_SERIAL.printf_P("[Timonel] - Data parsed with {{{ERROR}}} <<< Checksum = 0x%x\n\r", ackRX[1]);

      // USE_SERIAL.print(F("[Timonel] - Data parsed with {{{ERROR}}} <<< Checksum = 0x"));
      // USE_SERIAL.println(ackRX[1], HEX);
      //USE_SERIAL.println();
      if (commErrors++ > 0) {         /* Checksum error detected ... */
        USE_SERIAL.println(F("\n\r[Timonel] - WritePageBuff Checksum Errors, Aborting ..."));
        exit(commErrors);
      }
    }
  }
  else {

		USE_SERIAL.printf_P("[Timonel] - Error parsing %d command! <<< %d\n\r", cmdTX[0], ackRX[0]);

    // USE_SERIAL.print(F("[Timonel] - Error parsing "));
    // USE_SERIAL.print(cmdTX[0]);
    // USE_SERIAL.print(F(" command! <<< "));
    // USE_SERIAL.println(ackRX[0]);
    // USE_SERIAL.println();
    if (commErrors++ > 0) {         /* Opcode error detected ... */
      USE_SERIAL.println(F("\n\r[Timonel] - WritePageBuff Opcode Reply Errors, Aborting ..."));
      exit(commErrors);
    }
  }
  return(commErrors);
}

//Function ResetTiny
void ResetTiny(void) {
  USE_SERIAL.println(F("Sending ATtiny85 Reset Command ..."));
  byte cmdTX[1] = { RESETINY };
  byte txSize = sizeof(cmdTX);

	USE_SERIAL.printf_P("ESP8266 - Sending Opcode >>> %d (RESETINY)\n\r", cmdTX[0]);

  // USE_SERIAL.print(F("ESP8266 - Sending Opcode >>> "));
  // USE_SERIAL.print(cmdTX[0]);
  // USE_SERIAL.println(F("(RESETINY)"));
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

		USE_SERIAL.printf_P("ESP8266 - Command %d parsed OK <<< %d\n\r", cmdTX[0], ackRX[0]);

    // USE_SERIAL.print(F("ESP8266 - Command "));
    // USE_SERIAL.print(cmdTX[0]);
    // USE_SERIAL.print(F(" parsed OK <<< "));
    // USE_SERIAL.println(ackRX[0]);
  }
  else {

		USE_SERIAL.printf_P("ESP8266 - Error parsing %d command! <<< %d\n\r", cmdTX[0], ackRX[0]);

    // USE_SERIAL.print(F("ESP8266 - Error parsing "));
    // USE_SERIAL.print(cmdTX[0]);
    // USE_SERIAL.print(F(" command! <<< "));
    // USE_SERIAL.println(ackRX[0]);
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

 	USE_SERIAL.printf_P("\n\rGet Timonel Version >>> %d (GETTMNLV)\n\r", cmdTX[0]);

  // USE_SERIAL.print(F("\nGet Timonel Version >>> "));
  // USE_SERIAL.print(cmdTX[0]);
  // USE_SERIAL.println(F("(GETTMNLV)"));
  // Transmit command
  byte transmitData[1] = { 0 };
  for (byte i = 0; i < txSize; i++) {
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
    //trampolineJump = ((((timonelStart >> 1) - ++trampolineJump) & 0xFFF) << 1);
		trampolineJump++;
		trampolineJump = ((((timonelStart >> 1) - trampolineJump) & 0xFFF) << 1);
    //USE_SERIAL.print(F("[Timonel] - Command "));
    //USE_SERIAL.print(cmdTX[0]);
    //USE_SERIAL.print(F(" parsed OK <<< "));
    //USE_SERIAL.println(ackRX[0]);
    USE_SERIAL.println(F(" ____________________________________"));
    USE_SERIAL.println(F("| "));
    if (ackRX[1] == 84) {
      USE_SERIAL.print(F("| Timonel Bootloader v"));
    }

		USE_SERIAL.printf_P("%d.%d", ackRX[2], ackRX[3]);

    // USE_SERIAL.print(ackRX[2]);
    // USE_SERIAL.print(F("."));
    // USE_SERIAL.print(ackRX[3]);
    switch (ackRX[2]) {
      case 0: {
        USE_SERIAL.println(F(" Pre-release "));
        break;
      }
      case 1: {
        USE_SERIAL.println(F(" \"Sandra\" "));
        break;
      }
      default: {
        USE_SERIAL.println(F(" Unknown "));
        break;
      }
    }
    USE_SERIAL.printf_P("| ================================");

		USE_SERIAL.printf_P("| Bootloader address: 0x%04X\n\r", timonelStart);

    // USE_SERIAL.print(F("| Bootloader address: 0x"));
    // USE_SERIAL.print(timonelStart, HEX);
    // USE_SERIAL.println(F(" "));

		USE_SERIAL.printf_P("|  Application start: %02X%02X\n\r", ackRX[8], ackRX[7]);

    // USE_SERIAL.print(F("|  Application start: "));
    // USE_SERIAL.print(ackRX[8], HEX);
    // USE_SERIAL.print(ackRX[7], HEX);
    if ((ackRX[8] == 0xFF) && (ackRX[7] == 0xFF)) {
      USE_SERIAL.printf_P(" (Not Set)");
    }
    else {

			USE_SERIAL.printf_P(" (0x%04X)\n\r", trampolineJump);

      // USE_SERIAL.print(F(" (0x"));
      // USE_SERIAL.print(trampolineJump, HEX);
    }
    //USE_SERIAL.println(F(") "));
    //if (ackRX[9] == 0) {
    //  USE_SERIAL.println(F("|       Flash Memory: ** Clear **"));
    //  memoryLoaded = false;
    //}
    //else {
    //  USE_SERIAL.println(F("|       Flash Memory: >> Loaded <<"));
    //  memoryLoaded = true;
    //}

		USE_SERIAL.printf_P("|      Features Code: %d\n\r", ackRX[4]);

    // USE_SERIAL.print(F("|      Features Code: "));
    // USE_SERIAL.print(ackRX[4]);
    // USE_SERIAL.println(F(" "));
    USE_SERIAL.printf_P(" ____________________________________\n\r");
    //USE_SERIAL.println(F(""));
  }
  else {

		USE_SERIAL.printf_P("ESP8266 - Error parsing %d command! <<< %d\n\r", cmdTX[0], ackRX[0]);

    // USE_SERIAL.print(F("[Timonel] - Error parsing "));
    // USE_SERIAL.print(cmdTX[0]);
    // USE_SERIAL.print(F(" command! <<< "));
    // USE_SERIAL.println(ackRX[0]);
  }
}

// Function RunApplication
void RunApplication(void) {
  byte cmdTX[1] = { EXITTMNL };
  byte txSize = sizeof(cmdTX);

	USE_SERIAL.printf_P("\n[Timonel] Exit bootloader & run application >>> %d (EXITTMNL)\n\r", cmdTX[0]);

  // USE_SERIAL.print(F("\n[Timonel] Exit bootloader & run application >>> "));
  // //USE_SERIAL.print(F("ESP8266 - Sending Opcode >>> "));
  // USE_SERIAL.print(cmdTX[0]);
  // USE_SERIAL.println(F("(EXITTMNL)"));
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

		USE_SERIAL.printf_P("[Timonel] - Command %d parsed OK <<< %d\n\r", cmdTX[0], ackRX[0]);

    // USE_SERIAL.print(F("[Timonel] - Command "));
    // USE_SERIAL.print(cmdTX[0]);
    // USE_SERIAL.print(F(" parsed OK <<< "));
    // USE_SERIAL.println(ackRX[0]);
  }
  else {

		USE_SERIAL.printf_P("ESP8266 - Error parsing %d command! <<< %d\n\r", cmdTX[0], ackRX[0]);

    // USE_SERIAL.print(F("[Timonel] - Error parsing "));
    // USE_SERIAL.print(cmdTX[0]);
    // USE_SERIAL.print(F(" command! <<< "));
    // USE_SERIAL.println(ackRX[0]);
  }
}

// Function DeleteFlash
void DeleteFlash(void) {
  byte cmdTX[1] = { DELFLASH };
  byte txSize = sizeof(cmdTX);

	USE_SERIAL.printf_P("\n[Timonel] Delete Flash Memory >>> %d (DELFLASH)\n\r", cmdTX[0]);

  // USE_SERIAL.print(F("\n[Timonel] Delete Flash Memory >>> "));
  // //USE_SERIAL.print(F("ESP8266 - Sending Opcode >>> "));
  // USE_SERIAL.print(cmdTX[0]);
  // USE_SERIAL.println(F("(DELFLASH)"));
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

		USE_SERIAL.printf_P("[Timonel] - Command %d parsed OK <<< %d\n\r", cmdTX[0], ackRX[0]);

    // USE_SERIAL.print(F("[Timonel] - Command "));
    // USE_SERIAL.print(cmdTX[0]);
    // USE_SERIAL.print(F(" parsed OK <<< "));
    // USE_SERIAL.println(ackRX[0]);
  }
  else {

		USE_SERIAL.printf_P("ESP8266 - Error parsing %d command! <<< %d\n\r", cmdTX[0], ackRX[0]);

    // USE_SERIAL.print(F("[Timonel] - Error parsing "));
    // USE_SERIAL.print(cmdTX[0]);
    // USE_SERIAL.print(F(" command! <<< "));
    // USE_SERIAL.println(ackRX[0]);
  }
}

// Function SetTmlPageAddr
void SetTmlPageAddr(word pageAddr) {
 byte cmdTX[4] = { STPGADDR, 0, 0, 0 };
 byte txSize = 4;
 USE_SERIAL.println();
 cmdTX[1] = ((pageAddr & 0xFF00) >> 8);    /* Flash page address high byte */
 cmdTX[2] = (pageAddr & 0xFF);       /* Flash page address low byte */

 USE_SERIAL.printf_P("\n[Timonel] Setting flash page address on Attiny85 >>> %d (STPGADDR)\n\r", cmdTX[0]);

 // USE_SERIAL.print(F("\n[Timonel] Setting flash page address on Attiny85 >>> "));
 // USE_SERIAL.print(cmdTX[0]);
 // USE_SERIAL.println(F("(STPGADDR)"));
 cmdTX[3] = CalculateCRC(cmdTX, 2);
 // Transmit command
 byte transmitData[4] = { 0 };
 for (int i = 0; i < txSize; i++) {
   if (i > 0) {
     if (i < txSize - 1) {

			 USE_SERIAL.printf_P("[Timonel] - Sending Operand >>> %d\n\r", cmdTX[i]);

       // USE_SERIAL.print(F("[Timonel] - Sending Operand >>> "));
       // USE_SERIAL.println(cmdTX[i]);
     }
     else {

			 USE_SERIAL.printf_P("[Timonel] - Sending CRC >>> %d\n\r", cmdTX[i]);

       // USE_SERIAL.print(F("[Timonel] - Sending CRC >>> "));
       // USE_SERIAL.println(cmdTX[i]);
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

	 USE_SERIAL.printf_P("[Timonel] - Command %d parsed OK <<< %d\n\r", cmdTX[0], ackRX[0]);

   // USE_SERIAL.print(F("[Timonel] - Command "));
   // USE_SERIAL.print(cmdTX[0]);
   // USE_SERIAL.print(F(" parsed OK <<< "));
   // USE_SERIAL.println(ackRX[0]);
   if (ackRX[1] == (byte)(cmdTX[1] + cmdTX[2])) {

		 USE_SERIAL.printf_P("[Timonel] - Operands %d and %d parsed OK by slave <<< ATtiny85 Flash Page Address Check = %d\n\r", cmdTX[1], cmdTX[2], ackRX[1]);

     // USE_SERIAL.print(F("[Timonel] - Operands "));
     // USE_SERIAL.print(cmdTX[1]);
     // USE_SERIAL.print(F(" and "));
     // USE_SERIAL.print(cmdTX[2]);
     // USE_SERIAL.print(F(" parsed OK by slave <<< ATtiny85 Flash Page Address Check = "));
     // USE_SERIAL.println(ackRX[1]);
   }
   else {

		 USE_SERIAL.printf_P("[Timonel] - Operand %d parsed with {{{ERROR}}} <<< ATtiny85 Flash Page Address Check = %d\n\r", cmdTX[1], ackRX[1]);

     // USE_SERIAL.print(F("[Timonel] - Operand "));
     // USE_SERIAL.print(cmdTX[1]);
     // USE_SERIAL.print(F(" parsed with {{{ERROR}}} <<< ATtiny85 Flash Page Address Check = "));
     // USE_SERIAL.println(ackRX[1]);
   }

 }
 else {

	 USE_SERIAL.printf_P("[Timonel] - Error parsing %d command! <<< %d\n\r", cmdTX[0], ackRX[0]);

   USE_SERIAL.print(F("[Timonel] - Error parsing "));
   USE_SERIAL.print(cmdTX[0]);
   USE_SERIAL.print(F(" command! <<< "));
   USE_SERIAL.println(ackRX[0]);
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
 USE_SERIAL.println(F("\nWriting payload to flash ...\n\r"));
 //if (flashPageAddr == 0xFFFF) {
 //  USE_SERIAL.println(F("Warning: Flash page start address no set, please use 'b' command to set it ...\n\r"));
 //  return(1);
 //}
 //USE_SERIAL.print(F("::::::::::::::::::::::::::::::::::::::: Page "));
 //USE_SERIAL.print(pageCount);
 //USE_SERIAL.print(F(" - Address 0x"));
 //USE_SERIAL.println(flashPageAddr, HEX);
 for (int i = 0; i < payloadSize; i++) {
   if (i < (payloadSize - padding)) {
     dataPacket[packet] = payload[i];    /* If there are data to fill the page, use it ... */
   }
   else {
     dataPacket[packet] = 0xff;        /* If there are no more data, complete the page with padding (0xff) */
   }
   if (packet++ == (TXDATASIZE - 1)) {     /* When a data packet is completed to be sent ... */
     for (int b = 0; b < TXDATASIZE; b++) {
       //USE_SERIAL.print(F("0x"));
       //if (dataPacket[b] < 0x10) {
       //  USE_SERIAL.print(F("0"));
       //}
       //USE_SERIAL.print(dataPacket[b], HEX);
       //USE_SERIAL.print();
       USE_SERIAL.print(F("."));
     }
     wrtErrors += WritePageBuff(dataPacket); /* Send data to T85 through I2C */
     packet = 0;
     delay(10);                /* ###### DELAY BETWEEN PACKETS SENT TO PAGE ###### */
   }
   if (wrtErrors > 0) {
     //USE_SERIAL.println(F("\n\r==== WriteFlash: There were transmission errors, aborting ..."));
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

     USE_SERIAL.print(pageCount++);
     //DumpPageBuff(FLASHPGSIZE, TXDATASIZE, TXDATASIZE);
     delay(100);               /* ###### DELAY BETWEEN PAGE WRITINGS ... ###### */

     if (i < (payloadSize - 1)) {
       //USE_SERIAL.print(F("::::::::::::::::::::::::::::::::::::::: Page "));
       //USE_SERIAL.print(++pageCount);
       //USE_SERIAL.print(F(" - Address 0x"));
       //USE_SERIAL.println(((flashPageAddr + 1 + i) & 0xFFFF), HEX);
       pageEnd = 0;
     }
   }
 }
 if (wrtErrors == 0) {
   USE_SERIAL.println(F("\n\n\r==== WriteFlash: Firmware was successfully transferred to T85, please select 'run app' command to start it ..."));
 }
 else {

	 USE_SERIAL.printf_P("\n\n\r==== WriteFlash: Communication errors detected during firmware transfer, please retry !!! ErrCnt: %d ====\n\r", wrtErrors);

   // USE_SERIAL.print(F("\n\n\r==== WriteFlash: Communication errors detected during firmware transfer, please retry !!! ErrCnt: "));
   // USE_SERIAL.print(wrtErrors);
   // USE_SERIAL.println(F(" ==="));
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
    USE_SERIAL.print(F("Application command ('a', 's', 'z' reboot, 'x' reset T85, '?' help): "));
  }
  else {
    USE_SERIAL.print(F("Timonel booloader ('v' version, 'r' run app, 'e' erase flash, 'w' write flash, 'm' mem dump): "));
  }
}

//Function ShowHeader
void ShowHeader(void) {
	//USE_SERIAL.println();
	USE_SERIAL.println(F("\n\rTimonel Bootloader and Application I2C Commander Test (v1.2 Lanus)"));
	USE_SERIAL.println(F("==================================================================\n\r"));
	//USE_SERIAL.println();
}

//Function ShowTrampoline
void ShowTrampoline(void) {
#define TIMONEL_START 0x1A40
#define LSB 0x0E
#define MSB 0xC0
  USE_SERIAL.print(F("\nTIMONEL START = 0x"));
  USE_SERIAL.println(TIMONEL_START, HEX);
  USE_SERIAL.print(F("LSB = 0x"));
  USE_SERIAL.print(LSB, HEX);
  USE_SERIAL.print(F(" ||| MSB = 0x"));
  USE_SERIAL.println(MSB, HEX);
  word jumpOffset = ((MSB << 8) | LSB);
  USE_SERIAL.print(F("QQ = 0x"));
  USE_SERIAL.println(jumpOffset, HEX);
  //jumpOffset = (((~((TIMONEL_START >> 1) - (++jumpOffset & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
	jumpOffset++;
	jumpOffset = (((~((TIMONEL_START >> 1) - (jumpOffset & 0x0FFF)) + 1) & 0x0FFF) | 0xC000);
  USE_SERIAL.print(F("JUMP ADDRESS = 0x"));
  USE_SERIAL.println(jumpOffset, HEX);
}
